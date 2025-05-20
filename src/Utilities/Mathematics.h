#pragma once
#include <cmath>
#include <concepts>

namespace Weave
{
	namespace Mathematics
	{
		template <typename T>
		concept Numeric = std::is_arithmetic_v<T>;

        template <typename T>
        concept FloatingPoint = std::is_floating_point_v<T>;

        template <typename T>
        concept Integral = std::is_integral_v<T>;

		const double pi = 3.1415926535897932384;
        const float infinity = std::numeric_limits<float>::infinity();

        template<FloatingPoint ValueType>
        ValueType RadiansToDegrees(ValueType radians)
        {
            return radians * 180 / static_cast<ValueType>(pi);
        }

        template<FloatingPoint ValueType>
        ValueType DegreesToRadians(ValueType degrees)
        {
            return degrees * static_cast<ValueType>(pi) / 180;
        }

		template<Numeric T = float>
		struct Vector2
		{
		public:
			T x = (T)0;
			T y = (T)0;

            Vector2(T x, T y)
            {
                this->x = x;
                this->y = y;
            }

            Vector2()
            {
                this->x = (T)0;
                this->y = (T)0;
            }

            Vector2(const sf::Vector2<T>& in)
            {
                this->x = in.x;
                this->y = in.y;
            }

            template<Numeric CastType>
            operator Vector2<CastType>() const
            {
                return Vector2<CastType>(static_cast<CastType>(this->x), static_cast<CastType>(this->y));
            }

            float GetMagnitude()
            {
                return (float)std::sqrt(std::pow(this->x, 2) + std::pow(this->y, 2));
            }

            float GetSquaredMagnitude()
            {
                return (float)(std::pow(this->x, 2) + std::pow(this->y, 2));
            }

            Vector2<float> GetUnitVector()
            {
                if (this->GetSquaredMagnitude() == 0.0f) return { 0.0f, 0.0f };
                return *this / this->GetMagnitude();
            }

            Vector2<T> GetPerpendicularVector()
            {
                return Vector2<T>(y, -x);
            }

            float GetAngle()
            {
                if (this->GetSquaredMagnitude() == 0.0f) return 0;

                return RadiansToDegrees(std::atan2(this->x, this->y));
            }

            template<Numeric ArgType>
            Vector2<T>& operator *=(ArgType operand)
            {
                this->x *= operand;
                this->y *= operand;

                return *this;
            }

            template<Numeric ArgType>
            Vector2<T> operator *(ArgType operand)
            {
                return Vector2(static_cast<T>(this->x * operand), static_cast<T>(this->y * operand));
            }

            template<Numeric ArgType>
            Vector2<T>& operator *=(Vector2<ArgType> operand)
            {
                this->x *= operand.x;
                this->y *= operand.y;

                return *this;
            }

            template<Numeric ArgType>
            Vector2<T> operator *(Vector2<ArgType> operand)
            {
                return Vector2(static_cast<T>(this->x * operand.x), static_cast<T>(this->y * operand.y));
            }

            template<Numeric ArgType>
            Vector2<T>& operator /=(ArgType operand)
            {
                this->x /= operand;
                this->y /= operand;

                return *this;
            }

            template<Numeric ArgType>
            Vector2<T> operator /(ArgType operand)
            {
                return Vector2(static_cast<T>(this->x / operand), static_cast<T>(this->y / operand));
            }

            template<Numeric ArgType>
            Vector2<T>& operator /=(Vector2<ArgType> operand)
            {
                this->x /= operand.x;
                this->y /= operand.y;

                return *this;
            }

            template<Numeric ArgType>
            Vector2<T> operator /(Vector2<ArgType> operand)
            {
                return Vector2(static_cast<T>(this->x / operand.x), static_cast<T>(this->y / operand.y));
            }

            template<Numeric ArgType>
            Vector2<T>& operator +=(Vector2<ArgType> operand)
            {
                this->x += operand.x;
                this->y += operand.y;

                return *this;
            }

            template<Numeric ArgType>
            Vector2<T> operator +(Vector2<ArgType> operand)
            {
                return Vector2(static_cast<T>(this->x + operand.x), static_cast<T>(this->y + operand.y));
            }

            template<Numeric ArgType>
            Vector2<T>& operator -=(Vector2<ArgType> operand)
            {
                this->x -= operand.x;
                this->y -= operand.y;

                return *this;
            }

            template<Numeric ArgType>
            Vector2<T> operator -(Vector2<ArgType> operand)
            {
                return Vector2(static_cast<T>(this->x - operand.x), static_cast<T>(this->y - operand.y));
            }

            Vector2<T> operator-()
            {
                return Vector2<T>(-x, -y);
            }

            template<Numeric T>
            static float Angle(Vector2<T> pointA, Vector2<T> pointB)
            {
                Vector2<float> direction = pointB - pointA;
                return std::atan2(direction.x, direction.y);
            }

            template<Numeric T>
            static float Dot(Vector2<T> vectorA, Vector2<T> vectorB)
            {
                return vectorA.x * vectorB.x + vectorA.y * vectorB.y;
            }
		};

        template<Numeric T>
        std::ostream& operator<<(std::ostream& os, const Vector2<T>& vec)
        {
            return os << "(" << vec.x << ", " << vec.y << ")";
        }

		template<Numeric ArgType, FloatingPoint TimeType>
        ArgType Lerp(ArgType a, ArgType b, TimeType t)
		{
			return static_cast<ArgType>(a * (static_cast<TimeType>(1) - t) + (b * t));
		}

        template<Numeric ArgType, FloatingPoint TimeType>
        Vector2<ArgType> Lerp(Vector2<ArgType> a, Vector2<ArgType> b, TimeType t)
        {
            return static_cast<Vector2<ArgType>>(a * (static_cast<TimeType>(1) - t) + (b * t));
        }

		template<Numeric ValueType>
		ValueType Clamp(ValueType value, ValueType min, ValueType max)
		{
			return std::max(min, std::min(value, max));
		}

        template<Numeric ValueType>
        ValueType Abs(ValueType value)
        {
            return std::abs(value);
        }

        template<Numeric ValueType>
        Vector2<ValueType> Abs(Vector2<ValueType> value)
        {
            return { Abs(value.x), Abs(value.y) };
        }

        template<FloatingPoint ValueType>
        Vector2<ValueType> AngleToVector(ValueType angle)
        {
            float angleRadians = DegreesToRadians(angle);
            return { std::sin(angleRadians), std::cos(angleRadians) };
        }
	}
}