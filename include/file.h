#ifndef __simplelib_file_h__
#define __simplelib_file_h__

#include "stream.h"
#include "stringbuilder.h"
#include "encoding.h"

namespace SimpleLib
{

// Abstract Stream Class
template <typename T>
class CFile
{
public:

	// Reads a UTF-8 encoding text file into a CString
	template <typename TStr>
	static int ReadAllText(const T* filename, CString<TStr>& text)
	{
		// Clear text
		text.Clear();

		// Open file
		CFileStream file;
		int err = file.Open(filename);
		if (err)
			return err;

		// Setup encoder
		CStringBuilder<TStr> out;
		Encoding<char, TStr> enc;

		// Read it
		char buf[4096];
		while (true)
		{
			// Read
			size_t cb;
			int err = file.Read(buf, sizeof(buf), &cb);
			if (err != 0 && err != EOF)
				return err;

			// Encode
			for (size_t i=0; i<cb; i++)
			{
				enc.Process(buf[i], out);
			}

			// Quit if finished
			if (cb < sizeof(buf))
				break;
		}

		// Return result
		text = CString<TStr>(out);
		return 0;
	}

	// Writes a UTF-8 encoded text file
	template <typename TStr>
	static int WriteAllText(const T* filename, const TStr* text)
	{
		// Create the file
		CFileStream file;
		int err = file.Create(filename);
		if (err)
			return err;

		// Setup encoder
		Encoding<TStr, char> enc;
		CStreamStringWriter<char> w(file);

		// Encode and write
		const TStr* p = text;
		while (*p)
		{
			enc.Process(*p++, w);
		}

		return w.GetError();
	}

};

}	// namespace

#endif // __simplelib_file_h__