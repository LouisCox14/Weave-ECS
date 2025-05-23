#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <array>
#include <algorithm>
#include <span>

namespace Weave
{
	class ISparseSet
	{
	public:
		virtual std::size_t Size() = 0;
		virtual bool HasIndex(std::size_t index) = 0;
		virtual void Delete(std::size_t index) = 0;
	};

	template<typename T>
	struct SparseSet : public ISparseSet
	{
	private:
		static constexpr std::size_t SPARSE_PAGE_SIZE = 1024;

		std::vector<std::unique_ptr<std::array<std::size_t, SPARSE_PAGE_SIZE>>> sparsePages;
		std::vector<T> dense;
		std::vector<std::size_t> denseToSparse;

		struct PaginatedArrayIndex
		{
			std::size_t page;
			std::size_t index;

			PaginatedArrayIndex(std::size_t _page, std::size_t _index)
			{
				page = _page;
				index = _index;
			}
		};

		PaginatedArrayIndex GetSparseIndex(std::size_t index)
		{
			return PaginatedArrayIndex(index / SPARSE_PAGE_SIZE, index % SPARSE_PAGE_SIZE);
		}

		std::size_t GetDenseIndex(std::size_t index)
		{
			return GetDenseIndex(GetSparseIndex(index));
		}

		std::size_t GetDenseIndex(PaginatedArrayIndex sparseIndex)
		{
			if (sparseIndex.page >= sparsePages.size())
				return SIZE_MAX;

			if (!sparsePages[sparseIndex.page].get())
				return SIZE_MAX;

			return (*sparsePages[sparseIndex.page].get())[sparseIndex.index];
		}

		std::size_t* GetDenseIndexPtr(std::size_t index)
		{
			PaginatedArrayIndex sparseIndex = GetSparseIndex(index);

			if (sparseIndex.page >= sparsePages.size())
				return nullptr;

			if (!sparsePages[sparseIndex.page].get())
				return nullptr;

			return &(*sparsePages[sparseIndex.page].get())[sparseIndex.index];
		}

	public:
		void Set(std::size_t index, T data)
		{
			PaginatedArrayIndex sparseIndex = GetSparseIndex(index);

			if (sparsePages.size() <= sparseIndex.page)
			{
				sparsePages.resize(sparseIndex.page + 1);
			}

			if (!sparsePages[sparseIndex.page].get())
			{
				sparsePages[sparseIndex.page] = std::make_unique<std::array<std::size_t, SPARSE_PAGE_SIZE>>();
				sparsePages[sparseIndex.page].get()->fill(SIZE_MAX);
			}

			std::size_t currentDenseIndex = (*sparsePages[sparseIndex.page].get())[sparseIndex.index];

			if (currentDenseIndex == SIZE_MAX)
			{
				(*sparsePages[sparseIndex.page].get())[sparseIndex.index] = dense.size();
				denseToSparse.push_back(index);
				dense.push_back(data);
			}
			else
			{
				dense[currentDenseIndex] = data;
				denseToSparse[currentDenseIndex] = index;
			}
		}

		void Delete(std::size_t index) override
		{
			std::size_t denseIndex = GetDenseIndex(index);

			if (denseIndex == SIZE_MAX)
				return;

			*GetDenseIndexPtr(denseToSparse.back()) = denseIndex;
			*GetDenseIndexPtr(index) = SIZE_MAX;

			std::swap(dense.back(), dense[denseIndex]);
			std::swap(denseToSparse.back(), denseToSparse[denseIndex]);

			dense.pop_back();
			denseToSparse.pop_back();
		}

		T* Get(std::size_t index)
		{
			std::size_t denseIndex = GetDenseIndex(index);

			if (denseIndex == SIZE_MAX)
				return nullptr;

			return &dense[denseIndex];
		}

		bool HasIndex(std::size_t index) override
		{
			std::size_t denseIndex = GetDenseIndex(index);

			if (denseIndex == SIZE_MAX)
				return false;

			return true;
		}

		std::span<T> GetDenseView()
		{
			return std::span<T>(dense);
		}

		std::vector<std::size_t> GetIndexes()
		{
			return denseToSparse;
		}

		std::size_t Size() override
		{
			return dense.size();
		}
	};
}