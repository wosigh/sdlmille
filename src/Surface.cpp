/*
This file is part of SDL Mille.

SDL Mille is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

SDL Mille is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SDL Mille.  If not, see <http://www.gnu.org/licenses/>.

(See file LICENSE for full text of license)
*/

#include "Surface.h"

namespace _SDLMille
{

				Surface::Surface		(void)
{
	MySurface = 0;
	Cached = 0;
	Integer = 0;
	Length = 0;
}

				Surface::~Surface		(void)
{
	if (MySurface != 0)
		SDL_FreeSurface(MySurface);

	if (Cached != 0)
		delete [] Cached;
}

void			Surface::Clear			(void)
{
	if (MySurface != 0)
	{
		SDL_FreeSurface(MySurface);
		MySurface = 0;
	}

	if (Cached != 0)
	{
		delete [] Cached;
		Cached = 0;
		Length = 0;
	}
}

bool			Surface::Draw			(SDL_Surface * Destination, SDL_Surface * Source, int X, int Y, bool Free)
{
	if ((Destination == 0) || (Source == 0))
		return false;

	#ifdef	ANDROID_DEVICE
		X *= 1.5;
		Y *= 1.5;
		Y += 40;
	#endif

	SDL_Rect	DestRect;
	DestRect.x = X;
	DestRect.y = Y;

	SDL_BlitSurface(Source, 0, Destination, &DestRect);

	if (Free)
		SDL_FreeSurface(Source);

	return true;
}

int				Surface::GetHeight		(void)																	const
{
	if (MySurface != 0)
		return MySurface->h;

	return 0;
}

int				Surface::GetWidth		(void)																	const
{
	if (MySurface != 0)
		return MySurface->w;

	return 0;
}

SDL_Surface *	Surface::Load			(const char * File)
{
	if (File != 0)
	{
		SDL_Surface	*Loaded = 0,
					*Formatted = 0;

		// TODO: Fix this kludge
		#ifdef	ANDROID_DEVICE
		char	*NewFile = 0;
		NewFile = new	char[strlen(File) + 5];
		if (NewFile)
		{
			strcpy(NewFile, "hd/");
			strcat(NewFile, File);
			Loaded = IMG_Load(NewFile);
			delete	[]	NewFile;
			NewFile = 0;
		}
		#else
		Loaded = IMG_Load(File);
		#endif

		if (Loaded == 0)
			return 0;

		Formatted = SDL_DisplayFormatAlpha(Loaded);
		
		SDL_FreeSurface(Loaded);

		return Formatted;
	}
	else
		return 0;
}

void			Surface::Render			(int X, int Y, SDL_Surface * Destination)								const
{
	if (MySurface != 0)
		Draw(Destination, MySurface, X, Y);
}

SDL_Surface *	Surface::RenderText		(const char *Text, TTF_Font *Font, int R, int G, int B)
{
	SDL_Color	TextColor = {R, G, B, 0};
	SDL_Surface	*TextSurface = 0;

	if (Font != 0)
	{
		TextSurface = TTF_RenderText_Blended(Font, Text, TextColor);
		return TextSurface;
	}

	return 0;
}

void			Surface::SetImage		(const char * File)
{
	if (CheckCache(File))
	{
		if (MySurface != 0)
		{
			SDL_FreeSurface(MySurface);
			MySurface = 0;
		}

		MySurface = Load(File);
	}
}

void			Surface::SetInteger		(int Value, TTF_Font * Font, bool ShowZero)
{
	if ((Value < 0) || (Font == 0))
		return;

	if (CheckCache("INTEGER") || (Value != Integer))
	{
		Integer = Value;

		if (MySurface != 0)
		{
			SDL_FreeSurface(MySurface);
			MySurface = 0;
		}

		if (Value == 0)
		{
			if (ShowZero)
				MySurface = RenderText("0", Font, 0, 0, 0);
			else
				MySurface = RenderText("-", Font, 0, 0, 0);
		}
		else
		{
			int		NumDigits = 0,
					TempNum = Value;
			char	*MyText = 0;

			while (TempNum > 0)
			{
				++NumDigits;
				TempNum /= 10;
			}

			MyText = new char[NumDigits + 1];

			if ((MyText != 0) && (Value <= pow((double)10, NumDigits))) //Sanity check
			{
				sprintf(MyText, "%u", Value);
				MySurface = RenderText(MyText, Font, 0, 0, 0);
			}
		}
	}
}

void			Surface::SetText		(const char * Text, TTF_Font * Font, int R, int G, int B)
{
	if (Font == 0)
		return;

	if (CheckCache(Text))
	{
		if (MySurface != 0)
		{
			SDL_FreeSurface(MySurface);
			MySurface = 0;
		}

		MySurface = RenderText(Text, Font, R, G, B);
	}
}

/* Private methods */

bool			Surface::CheckCache		(const char * Text)
{
	bool	CacheDirty = false;

	if (Text == 0)
		return false;

	if (Cached != 0)
	{
		if (strcmp(Cached, Text))
		{
			CacheDirty = true;
			delete [] Cached;
			Cached = 0;
			Length = 0;
		}
	}
	else
		CacheDirty = true;

	if (CacheDirty)
	{
		Length = strlen(Text);
		Cached = new char[Length + 1];
		strcpy(Cached, Text);
	}

	return CacheDirty;
}

}
