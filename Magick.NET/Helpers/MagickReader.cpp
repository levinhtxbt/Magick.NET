//=================================================================================================
// Copyright 2013 Dirk Lemstra <http://magick.codeplex.com/>
//
// Licensed under the ImageMagick License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
//
//   http://www.imagemagick.org/script/license.php
//
// Unless required by applicable law or agreed to in writing, software distributed under the
// License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing permissions and
// limitations under the License.
//=================================================================================================
#include "stdafx.h"
#include "MagickReader.h"
#include "FileHelper.h"

namespace ImageMagick
{
	//===========================================================================================
	void MagickReader::Read(Magick::Blob* blob, array<Byte>^ data, int length)
	{
		char* unmanagedValue = new char[length];
		Marshal::Copy(data, 0, IntPtr(unmanagedValue), length);
		blob->update(unmanagedValue, length);
		delete[] unmanagedValue;
	}
	//==============================================================================================
	void MagickReader::SetSize(Magick::Image* image, Nullable<int> width, Nullable<int> height)
	{
		if (!width.HasValue || !height.HasValue)
			return;

		Magick::Geometry geometry = Magick::Geometry(width.Value, height.Value);
		image->size(geometry);
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, bool ping, Magick::Blob* blob,
		Nullable<int> width, Nullable<int> height, Nullable<ColorSpace> colorSpace)
	{
		try
		{
			if (ping)
			{
				image->ping(*blob);
			}
			else
			{
				SetSize(image, width, height);
				image->read(*blob);
			}

			if (colorSpace.HasValue)
				SetColorSpace(image, colorSpace.Value);
		}
		catch (Magick::Warning& exception)
		{
			return MagickWarningException::Create(exception);
		}
		catch (Magick::Exception& exception)
		{
			throw MagickException::Create(exception);
		}

		return nullptr;
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, bool ping, Stream^ stream,
		Nullable<int> width, Nullable<int> height, Nullable<ColorSpace> colorSpace)
	{
		Magick::Blob blob;
		Read(&blob, stream);
		return Read(image, ping, &blob, width, height, colorSpace);
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, bool ping, String^ fileName,
		Nullable<int> width, Nullable<int> height, Nullable<ColorSpace> colorSpace)
	{
		String^ filePath = FileHelper::CheckForBaseDirectory(fileName);
		Throw::IfInvalidFileName(filePath);

		try
		{
			std::string imageSpec;
			Marshaller::Marshal(filePath, imageSpec);

			if (ping)
			{
				image->ping(imageSpec);
			}
			else
			{
				SetSize(image, width, height);
				image->read(imageSpec);
			}

			if (colorSpace.HasValue)
				SetColorSpace(image, colorSpace.Value);
		}
		catch (Magick::Warning& exception)
		{
			return MagickWarningException::Create(exception);
		}
		catch (Magick::Exception& exception)
		{
			throw MagickException::Create(exception);
		}

		return nullptr;
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, Magick::Blob* blob,
		Nullable<ColorSpace> colorSpace)
	{
		try
		{
			Magick::readImages(imageList, *blob);

			if (colorSpace.HasValue)
				SetColorSpace(imageList, colorSpace.Value);
		}
		catch (Magick::Warning& exception)
		{
			return MagickWarningException::Create(exception);
		}
		catch (Magick::Exception& exception)
		{
			throw MagickException::Create(exception);
		}

		return nullptr;
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, Stream^ stream,
		Nullable<ColorSpace> colorSpace)
	{
		Magick::Blob blob;
		Read(&blob, stream);
		return Read(imageList, &blob, colorSpace);
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, String^ fileName,
		Nullable<ColorSpace> colorSpace)
	{
		String^ filePath = FileHelper::CheckForBaseDirectory(fileName);
		Throw::IfInvalidFileName(filePath);

		try
		{
			std::string imageSpec;
			Marshaller::Marshal(filePath, imageSpec);

			Magick::readImages(imageList, (std::string)imageSpec);

			if (colorSpace.HasValue)
				SetColorSpace(imageList, colorSpace.Value);
		}
		catch (Magick::Warning& exception)
		{
			return MagickWarningException::Create(exception);
		}
		catch (Magick::Exception& exception)
		{
			throw MagickException::Create(exception);
		}

		return nullptr;
	}
	//==============================================================================================
	void MagickReader::SetColorSpace(Magick::Image* image, ColorSpace colorSpace)
	{
		MagickCore::ColorspaceType colorSpaceType = (MagickCore::ColorspaceType)colorSpace;

		if (image->colorSpace() == colorSpaceType)
			return;

		if (image->colorSpace() == MagickCore::ColorspaceType::CMYKColorspace &&
			(colorSpace == ColorSpace::RGB || colorSpace == ColorSpace::sRGB))
		{
			image->profile("ICM", (Magick::Blob&)ColorProfile::SRGB);
		}

		image->colorSpace((MagickCore::ColorspaceType)colorSpace);
	}
	//==============================================================================================
	void MagickReader::SetColorSpace(std::list<Magick::Image>* imageList, ColorSpace colorSpace)
	{
		for(std::list<Magick::Image>::iterator iter = imageList->begin(); iter != imageList->end(); iter++)
		{
			SetColorSpace(&*(iter), colorSpace);
		}
	}
	//===========================================================================================
	void MagickReader::Read(Magick::Blob* blob, array<Byte>^ data)
	{
		Throw::IfNull("data", data);

		Read(blob, data, data->Length);
	}
	//===========================================================================================
	void MagickReader::Read(Magick::Blob* blob, Stream^ stream)
	{
		Throw::IfNull("stream", stream);

		array<Byte>^ data = nullptr;

		int length = 0;

		if (stream->CanSeek)
		{
			length = (int)stream->Length;
			data = gcnew array<Byte>(length);
			stream->Read(data, 0, length);
		}
		else
		{
			int bufferSize = 8192;
			data = gcnew array<Byte>(bufferSize);

			int readLength;
			while ((readLength = stream->Read(data, length, bufferSize)) > 0)
			{
				Array::Resize(data, data->Length + bufferSize);
				length += readLength;
			}
		}

		Read(blob, data, length);
	}
	//===========================================================================================
	void MagickReader::Read(Magick::Blob* blob, String^ fileName)
	{
		String^ filePath = FileHelper::CheckForBaseDirectory(fileName);
		Throw::IfInvalidFileName(filePath);

		FileStream^ stream = File::OpenRead(filePath);
		Read(blob, stream);
		delete stream;
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, MagickBlob^ blob, bool ping)
	{
		return Read(image, ping, (Magick::Blob*)blob, Nullable<int>(), Nullable<int>(), Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, MagickBlob^ blob,
		ColorSpace colorSpace, bool ping)
	{
		return Read(image, ping, (Magick::Blob*)blob, Nullable<int>(), Nullable<int>(), Nullable<ColorSpace>(colorSpace));
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, MagickBlob^ blob,
		int width, int height)
	{
		return Read(image, false, (Magick::Blob*)blob, width, height, Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, Stream^ stream, bool ping)
	{
		return Read(image, ping, stream, Nullable<int>(), Nullable<int>(), Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, Stream^ stream,
		ColorSpace colorSpace, bool ping)
	{
		return Read(image, ping, stream, Nullable<int>(), Nullable<int>(), Nullable<ColorSpace>(colorSpace));
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, Stream^ stream,
		int width, int height)
	{
		return Read(image, false, stream, width, height, Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, String^ fileName, bool ping)
	{
		return Read(image, ping, fileName, Nullable<int>(), Nullable<int>(), Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, String^ fileName,
		ColorSpace colorSpace, bool ping)
	{
		return Read(image, ping, fileName, Nullable<int>(), Nullable<int>(), Nullable<ColorSpace>(colorSpace));
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(Magick::Image* image, String^ fileName,
		int width, int height)
	{
		return Read(image, false, fileName, width, height, Nullable<ColorSpace>());
	}
	//==============================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, MagickBlob^ blob)
	{
		return Read(imageList, (Magick::Blob*)blob, Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, MagickBlob^ blob,
		ColorSpace colorSpace)
	{
		return Read(imageList, (Magick::Blob*)blob, Nullable<ColorSpace>(colorSpace));
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, Stream^ stream)
	{
		return Read(imageList, stream, Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, Stream^ stream,
		ColorSpace colorSpace)
	{
		return Read(imageList, stream, Nullable<ColorSpace>(colorSpace));
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, String^ fileName)
	{
		return Read(imageList, fileName, Nullable<ColorSpace>());
	}
	//===========================================================================================
	MagickWarningException^ MagickReader::Read(std::list<Magick::Image>* imageList, String^ fileName,
		ColorSpace colorSpace)
	{
		return Read(imageList, fileName, Nullable<ColorSpace>(colorSpace));
	}
	//==============================================================================================
}