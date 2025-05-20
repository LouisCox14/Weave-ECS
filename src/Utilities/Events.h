#pragma once
#include <queue>
#include <map>
#include <functional>
#include <concepts>

namespace Weave
{
	namespace Utilities
	{
		template<typename ListenerType, typename FunctionType, typename... ArgTypes>
		concept ValidMemberFunction = requires(FunctionType f, ListenerType & l, ArgTypes... args)
		{
			// This concept ensures that functions subscribing to the event can be called and do have the correct arguments and return type.
			{ std::invoke(f, l, args...) };
		};

		template<typename FunctionType, typename... ArgTypes>
		concept ValidFunction = requires(FunctionType f, ArgTypes... args)
		{
			// This concept ensures that functions subscribing to the event can be called and do have the correct arguments and return type.
			{ std::invoke(f, args...) };
		};

		template<typename... ArgTypes>
		class Event
		{
			/*
				This class is an implementation of the observer pattern. Events can be created with a return type and arguments, and functions that match that signature
				can be bound to the event to be called when it is invoked.
			*/

		private:
			std::map<std::tuple<size_t, size_t>, std::function<void(ArgTypes...)>> callbackMap;

			bool isRunning = false;

		public:
			void clear()
			{
				callbackMap.clear();
			}

			template<class ListenerType, class FunctionType>
			requires ValidMemberFunction<ListenerType, FunctionType, ArgTypes...>
			void Subscribe(ListenerType& listener, FunctionType&& function)
			{
				// This function lets other functions be bound to the event, meaning they are called when the event is invoked.

				static_assert(!std::is_pointer<ListenerType>::value, "Listener cannot be passed as a pointer.");

				// This solution feels unbelievably hacky, but storing the memory addresses of the function and the caller as ints is the only consistent way I could think
				// of to identify them later when they need to be unsubscribed.

				if (isRunning) throw std::logic_error("Cannot subscribe to an event whilst it is running.");

				callbackMap.insert({ { (size_t)&listener, *(size_t*)(char*)&function }, std::bind_front(function, std::ref(listener)) });
			}

			template <typename FunctionType>
			requires ValidFunction<FunctionType, ArgTypes...>
			void Subscribe(FunctionType&& function)
			{
				// This override ensures that static and non member functions can be used with the event class.

				if (isRunning) throw std::logic_error("Cannot subscribe to an event whilst it is running.");

				callbackMap.insert({ { 0, *(size_t*)(char*)&function }, function });
			}

			template<class ListenerType, class FunctionType>
			requires ValidMemberFunction<ListenerType, FunctionType, ArgTypes...>
			void Unsubscribe(ListenerType& listener, FunctionType&& function)
			{
				// This function lets functions bound to this event unsubscribe so that they are no longer called when it is invoked.

				static_assert(!std::is_pointer<ListenerType>::value, "Listener cannot be passed as a pointer.");

				if (isRunning) throw std::logic_error("Cannot unsubscribe from an event whilst it is running.");

				std::tuple callbackKey = { (size_t)&listener, *(size_t*)(char*)&function };

				if (!callbackMap.contains(callbackKey)) return;

				callbackMap.erase(callbackKey);
			}

			template<class FunctionType>
			requires ValidFunction<FunctionType, ArgTypes...>
			void Unsubscribe(FunctionType&& function)
			{
				if (isRunning) throw std::logic_error("Cannot unsubscribe from an event whilst it is running.");

				std::tuple callbackKey = { 0, *(size_t*)(char*)&function };

				if (!callbackMap.contains(callbackKey)) return;

				callbackMap.erase(callbackKey);
			}

			template<class ListenerType, class FunctionType>
			requires ValidMemberFunction<ListenerType, FunctionType, ArgTypes...>
			bool IsSubscribed(ListenerType& listener, FunctionType&& function)
			{
				static_assert(!std::is_pointer<ListenerType>::value, "Listener cannot be passed as a pointer.");

				return callbackMap.find(std::tuple<size_t, size_t>((size_t)&listener, *(size_t*)(char*)&function)) != callbackMap.end();
			}

			void operator ()(ArgTypes... args)
			{
				if (isRunning) throw std::logic_error("Cannot call an event that is already running.");

				isRunning = true;

				// This function goes through the map of functions subscribed to this event and calls them all.

				for (std::pair<std::tuple<size_t, size_t>, std::function<void(ArgTypes...)>> callback : callbackMap)
				{
					/*
						There is absolutely no checking here to make sure that the function I'm trying to call still exists, or that it's caller does.I'm not entirely sure
						how I could do that without dealing with overhead from exceptions, so for now I'll just leave it as is, and classes using events will have to 
						unsubscribe in their destructor. 
					*/

					callback.second(args...);
				}

				isRunning = false;
			}
		};
	}
}