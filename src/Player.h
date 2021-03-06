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

#ifndef _SDLMILLE_PLAYER_H
#define	_SDLMILLE_PLAYER_H

#include "Hand.h"
#include "Tableau.h"

namespace _SDLMille
{

const int	PLAYER_GIVEN_NO_ARGUMENT =	3000;

class Player
{
public:
				Player				(void);
	//bool		AnimationRunning	(void)					const;
	Uint8		CardsInHand			(void)					const;
	bool		Discard			(Uint8 Index);
	void		Detach			(Uint8 Index);
	void		Draw			(void);
	bool		Draw			(Uint8 Index);
	int			GetMileage		(void)						const;
	Uint8		GetPileCount	(Uint8 Value)				const;
	Uint8		GetTopCard		(bool SpeedPile = false)	const;
	Uint8		GetType			(Uint8 Index)				const;
	Uint8		GetQualifiedCoupFourre	(void)				const;
	Uint8		GetValue		(Uint8 Index)				const;
	bool		HasCoupFourre	(Uint8 Value)				const;
	bool		HasSafety		(Uint8 Value)				const;
	bool		IsDirty			(void)						const;
	bool		IsLimited		(void)						const;
	bool		IsOutOfCards	(void)						const;
	bool		IsPopped		(Uint8 Index)				const;
	bool		IsRolling		(void)						const;
	int			MileageInHand	(void)						const;
	Uint8		OnPlay			(Uint8 Index);
	bool		OnRender		(SDL_Surface * Target, Uint8 PlayerIndex, bool Force = false);
	bool		OnRenderHand	(SDL_Surface *Target, bool Force = false);
	void		Pop				(Uint8 Index);
	bool		ReceiveHazard	(Uint8 Value);
	void		Reset			(void);
	bool		Restore			(FILE *SaveFile);
	bool		Save			(FILE *SaveFile);
	void		SetSource		(Deck * ArgSource);
	void		UnPop			(Uint8 Index);
	void		UpdateTopCard	(bool IncludeRollCard, bool IncludeEndLimit);
private:
	Tableau		MyTableau;
	Hand		PlayerHand;
	Uint8		QualifiedCoupFourre;
	Deck *		SourceDeck;
};

//inline	bool	Player::AnimationRunning	(void)			const
//{
//	return MyTableau.AnimationRunning();
//}

inline	void	Player::Detach			(Uint8 Index)
{
	PlayerHand.Detach(Index);
}

inline	Uint8	Player::GetPileCount	(Uint8 Value)		const
{
	return MyTableau.GetPileCount(Value);
}

inline	int		Player::GetMileage		(void)				const
{
	return MyTableau.GetMileage();
}

inline	Uint8	Player::GetTopCard		(bool SpeedPile)	const
{
	return MyTableau.GetTopCard(SpeedPile);
}

inline	Uint8	Player::GetType			(Uint8 Index)		const
{
	return PlayerHand.GetType(Index);
}

inline	Uint8	Player::GetQualifiedCoupFourre	(void)		const
{
	return QualifiedCoupFourre;
}

inline	Uint8	Player::GetValue		(Uint8 Index)		const
{
	return PlayerHand.GetValue(Index);
}

inline	bool	Player::HasCoupFourre	(Uint8 Value)		const
{
	return MyTableau.HasCoupFourre(Value);
}

inline	bool	Player::HasSafety		(Uint8 Value)		const
{
	return MyTableau.HasSafety(Value);
}

inline	bool	Player::IsDirty			(void)				const
{
	return (PlayerHand.IsDirty() || MyTableau.IsDirty());
}

inline	bool	Player::IsLimited		(void)				const
{
	return MyTableau.HasSpeedLimit();
}

inline	bool	Player::IsPopped		(Uint8 Index)		const
{
	return PlayerHand.IsPopped(Index);
}

inline	bool	Player::IsRolling		(void)				const
{
	return MyTableau.IsRolling();
}

inline	void	Player::Pop				(Uint8 Index)
{
	PlayerHand.Pop(Index);
}

inline	void	Player::UnPop			(Uint8 Index)
{
	PlayerHand.UnPop(Index);
}

inline	void	Player::UpdateTopCard	(bool IncludeRollCard, bool IncludeEndLimit)
{
	MyTableau.UpdateTopCard(IncludeRollCard, IncludeEndLimit);
}

}

#endif
