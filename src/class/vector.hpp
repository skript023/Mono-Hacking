#pragma once
#include <numbers>

namespace big
{
	struct iVector2
	{
		iVector2() = default;
		iVector2(int x, int y) : x(x), y(y) {}
		int x{};
		int y{};

		iVector2 operator/(const iVector2 a) const { return { x / a.x, y / a.y }; }
	};

	struct iVector3
	{
		iVector3() = default;
		iVector3(int x, int y, int z) : x(x), y(y), z(z) {}
		int x{};
		int y{};
		int z{};
	};

	struct iVector4
	{
		int x;
		int y;
		int z;
		int w;
	};

	struct Vector2
	{
		Vector2() = default;
		Vector2(float x, float y) : x(x), y(y) {}
		float x;
		float y;

		float distance(const Vector2 b) const
		{
			float x = this->x - b.x;
			float y = this->y - b.y;
			return sqrtf((x * x) + (y * y)) * 0.03048f;
		}

		float dot(Vector2 vec2)
		{
			return (x * vec2.x) + (y * vec2.y);
		}

		void operator=(nlohmann::json const& data)
		{
			*this = data.get<Vector2>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector2, x, y)
	};

	struct Vector3
	{
		Vector3() = default;
		Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
		float x{};
		float y{};
		float z{};

		float dot(Vector3 other)
		{
			return (x * other.x) + (y * other.y) + (z * other.z);
		}

		float length() const
		{
			return std::sqrt((x * x) + (y * y) + (z * z));
		}

		float distance(Vector3 const& other) const
		{
			Vector3 diff = other - *this;
	
			return diff.magnitude();
		}

		float distance_in_meters(const Vector3& other) const
		{
			return distance(other) * 0.03048f;
		}

		bool is_zero() const
		{
			return x == 0.f && y == 0.f && z == 0.f;
		}

		Vector3 normalize() const noexcept
		{
			return *this / magnitude();
		}

		float magnitude() const
		{
			return std::sqrt((x * x) + (y * y) + (z * z));
		}

		void to_directions( Vector3* forward, Vector3* right, Vector3* up ) const noexcept
		{
			constexpr auto deg_to_rad = std::numbers::pi_v<float> / 180.0f;

			const auto sp = std::sinf( x * deg_to_rad );
			const auto cp = std::cosf( x * deg_to_rad );
			const auto sy = std::sinf( y * deg_to_rad );
			const auto cy = std::cosf( y * deg_to_rad );
			const auto sr = std::sinf( z * deg_to_rad );
			const auto cr = std::cosf( z * deg_to_rad );

			if ( forward )
			{
				forward->x = cp * cy;
				forward->y = cp * sy;
				forward->z = -sp;
			}

			if ( right )
			{
				right->x = -1.0f * sr * sp * cy + -1.0f * cr * -sy;
				right->y = -1.0f * sr * sp * sy + -1.0f * cr * cy;
				right->z = -1.0f * sr * cp;
			}

			if ( up )
			{
				up->x = cr * sp * cy + -sr * -sy;
				up->y = cr * sp * sy + -sr * cy;
				up->z = cr * cp;
			}
		}

		bool operator!=(const Vector3& other) const
		{
			return x != other.x || y != other.y || z != other.z;
		}
		Vector3 operator*(float scalar) const
		{
			return { x * scalar, y * scalar, z * scalar };
		}
		Vector3 operator*(const Vector3& other) const
		{
			return { x * other.x, y * other.y, z * other.z };
		}
		Vector3 operator+(const Vector3& other) const
		{
			return { x + other.x, y + other.y, z + other.z };
		}
		Vector3 operator-(const Vector3& other) const
		{
			return { x - other.x, y - other.y, z - other.z };
		}
		Vector3 operator/(float Scalar) const
		{
			if (Scalar == 0)
				return *this;

			return { x / Scalar, y / Scalar, z / Scalar };
		}
		Vector3 operator/(const Vector3& other) const
		{
			if (other.x == 0 || other.y == 0 || other.z == 0)
				return *this;

			return { x / other.x, y / other.y, z / other.z };
		}
		bool operator==(const Vector3& other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}

		void operator=(nlohmann::json const& data)
		{
			*this = data.get<Vector3>();
		}

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector3, x, y, z)
	};

	struct Rotator
	{
		Rotator() = default;
		Rotator(float pitch, float yaw, float roll) : pitch(pitch), yaw(yaw), roll(roll) {}
		float pitch{};
		float yaw{};
		float roll{};

		Rotator operator-(const Rotator vec3) const { return { vec3.pitch - pitch, vec3.yaw - yaw, vec3.roll - roll }; }
		Rotator operator*(const Rotator& a) const { return { pitch * a.pitch, yaw * a.yaw, roll * a.roll }; }
		Rotator operator+(const Rotator& vec3) const { return { pitch + vec3.pitch, yaw * vec3.yaw, roll * vec3.roll }; }
		Rotator operator/(const Rotator& vec3) const { return { vec3.pitch / pitch, vec3.yaw / yaw, vec3.roll / roll }; }
		bool operator==(const Rotator a) const { return pitch == a.pitch && yaw == a.yaw && roll == a.roll; }
	};

	struct Vector4
	{
		Vector4() = default;
		Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		float x{};
		float y{};
		float z{};
		float w{};

		float dot(Vector4 vec4)
		{
			return (x * vec4.x) + (y * vec4.y) + (z * vec4.z) + (w * vec4.w);
		}

		float bitwise_and_first(int v)
		{
			return float((int)x & v);
		}

		Vector4 operator&(const Vector4& v) { return { float((int)v.x & (int)x), float((int)v.y & (int)y), float((int)v.z & (int)z), float((int)v.w & (int)w) }; };
	};

	struct Bounds
	{
		Vector3 center;   // m_Center
		Vector3 extents;  // m_Extents
	};

	using FVector = Vector3;
	using FVector2D = Vector2;
	using Quaternions = Vector4;
}