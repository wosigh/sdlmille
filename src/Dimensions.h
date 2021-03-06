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

#ifndef	_SDLMILLE_DIMENSIONS_H
#define	_SDLMILLE_DIMENSIONS_H

#include <stdlib.h>
#include <algorithm>

namespace _SDLMille
{

const	int	SCREEN_EDGE_PADDING = 3,
			TRAY_TOP_BOTTOM_PADDING = 5;

class	Dimensions
{
public:
	static	void	SetDimensions		(int Width, int Height, int CardWidth, int CardHeight, bool VerticalTray);
	static	void	SetGamePlayMetrics	(int CardWidth, int CardHeight, bool VerticalTray);
	static	void	SetMenuMetrics		(int MenuWidth, int MenuHeight);
	static	void	SetTableauMetrics	(void);

	static	double	ScaleFactor;
	static	int		EffectiveTableauHeight,
					FirstRowY,
					GamePlayCardHeight,
					GamePlayCardSpacingX,
					GamePlayCardSpacingY,
					GamePlayCardsPerRow,
					GamePlayCardWidth,
					GamePlayHandLeftX,
					GamePlayTableauWidth,
					MenuBorderPadding,
					MenuColumn1X,
					MenuColumn2X,
					MenuItemSpacing,
					MenuItemsTopY,
					MenuX,
					MenuY,
					PadLeft,
					SecondRowY,
					ScreenWidth,
					ScreenHeight,
					TableauBattleX,
					TableauHeight,
					TableauLimitX,
					TableauPileSpacing,
					TableauSpacingX,
					TableauSpacingY,
					TrayDiscardX,
					TrayDiscardY,
					TrayDrawX,
					TrayDrawY;
	static	bool	LandscapeMode,
					MultiRowSafeties,
					GamePlayMultiRowTray;
};

}

#endif
