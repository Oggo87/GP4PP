#pragma once

#include <array>
#include <initializer_list>
#include <windows.h>

namespace GP4PP
{
	class BGRA
	{
	private:
		std::array<short,4> bytes = { 0, 0, 0, 0 };
		DWORD colour = 0x00000000;

	public:
		//Default constructor
		BGRA()
		{
		}

		//Copy constructor
		BGRA(const BGRA& other)
		{
			colour = other.colour;
			bytes = other.bytes;
		}

		//Construct from DWORD colour
		BGRA(DWORD col)
		{
			setColour(col);
		}

		//Construct from individual bytes
		BGRA(short b, short g, short r, short a)
		{
			setColour(b, g, r, a);
		}

		//Construct from std::array
		BGRA(std::array<short, 4> bytes)
		{
			setColour(bytes);
		}

		//Construct from std::vector
		BGRA(std::vector<short> bytes)
		{
			setColour(bytes);
		}

		//getters and setters
		DWORD getColour() const
		{
			return colour;
		}

		const DWORD& getColourRef() const
		{
			return colour;
		}

		// Individual component getters (B, G, R, A)
		short getB() const
		{
			if (bytes.size() > 0) return bytes[0];
			return static_cast<short>(colour & 0xFF);
		}

		short getG() const
		{
			if (bytes.size() > 1) return bytes[1];
			return static_cast<short>((colour >> 8) & 0xFF);
		}

		short getR() const
		{
			if (bytes.size() > 2) return bytes[2];
			return static_cast<short>((colour >> 16) & 0xFF);
		}

		short getA() const
		{
			if (bytes.size() > 3) return bytes[3];
			return static_cast<short>((colour >> 24) & 0xFF);
		}

		//set from individual bytes
		void setColour(short b, short g, short r, short a)
		{
			std::array<short, 4> byteArray = { b, g, r, a };
			setColour(byteArray);
		}

		//set from std::array
		void setColour(std::array<short, 4> newBytes)
		{
			bytes = newBytes;
			colour = 0;
			// Ensure we only use the first 4 bytes
			for (unsigned int i = 0; i < bytes.size() && i < 4; i++)
			{
				if (bytes[i] > 255)
				{
					bytes[i] = 255;
				}
				colour += (bytes[i] << i * 8);
			}
		}

		//set from std::vector
		void setColour(std::vector<short> newBytes)
		{
			std::array<short, 4> byteArray = { 0, 0, 0, 0 };
			std::copy_n(newBytes.begin(), std::min<size_t>(newBytes.size(), 4), byteArray.begin());
			setColour(byteArray);
		}

		//set from DWORD colour
		void setColour(DWORD newColour)
		{
			colour = newColour;
			bytes.fill(0);
			for (unsigned int i = 0; i < 4; i++)
			{
				bytes[i] = static_cast<short>((newColour >> (i * 8)) & 0xFF);
			}
		}

		// Assignment operator (copy)
		BGRA& operator=(const BGRA& other)
		{
			if (this != &other)
			{
				colour = other.colour;
				bytes = other.bytes;
			}
			return *this;
		}

		// Assignment operator from DWORD
		BGRA& operator=(DWORD newColour)
		{
			setColour(newColour);
			return *this;
		}

		// Assignment operator from initializer_list
		BGRA& operator=(std::initializer_list<short> init)
		{
			std::array<short, 4> v = { 0, 0, 0, 0 };
			std::copy_n(init.begin(), std::min<size_t>(init.size(), 4), v.begin());
			setColour(v);
			return *this;
		}

		// Assignment operator from byte array
		BGRA& operator=(const std::array<short, 4>& newBytes)
		{
			setColour(newBytes);
			return *this;
		}

		// Comparison operators
		bool operator==(const BGRA& other) const
		{
			return colour == other.colour;
		}

		bool operator!=(const BGRA& other) const
		{
			return colour != other.colour;
		}

		// Conversion operator to const reference of internal colour
		operator const DWORD& () const
		{
			return colour;
		}


	};
}

