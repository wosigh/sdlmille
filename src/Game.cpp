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

#include "Game.h"

namespace _SDLMille
{
/*Public methods */

		Game::Game				(void)
{
	Current = 0;	// Set current player to Player 1

	// Set our deck pointer to zero, then try to allocate
	SourceDeck = 0;
	SourceDeck = new Deck;

	// Initialize remaining pointers to zero
	Window = 0;
	GameOverSmall = GameOverBig = DrawFont = 0;

	Modal = MODAL_NONE;
	Scene = SCENE_MAIN;
	LastScene = SCENE_INVALID;
	DownIndex = 0xFF;

	Animating = false;
	Dirty = true;
	Dragging = false;
	Extended = false;
	ExtensionDeclined = false;
	Frozen = false;
	HumanWon = false;
	MouseDown = false;
	Running = true;

	OldDiscardTop = DiscardTop = CARD_NULL_NULL;

	FrozenAt = 0;
	MessagedAt = 0;

	// Initialize SDL_ttf
	if (!TTF_WasInit())
		TTF_Init();

	// Tell all of our players to use our deck
	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		if (SourceDeck)
			Players[i].SetSource(SourceDeck);

		// And initialize their scores to zero
		Scores[i] = 0;
		RunningScores[i] = 0;

		for (int j = 0; j < SCORE_CATEGORY_COUNT; ++j)
			ScoreBreakdown[i][j] = 0;
	}

	//Staggered deal
	for (int i = 0; i < 13; ++i)
		Players[i % 2].Draw();

	if (SourceDeck)
		OldDeckCount = DeckCount = SourceDeck->CardsLeft();

	GameOptions.ReadOpts();

	Message[0] = '\0';

	DrawFont = TTF_OpenFont("LiberationMono-Regular.ttf", 16);
	GameOverBig = TTF_OpenFont("LiberationMono-Regular.ttf", 18);
	GameOverSmall = TTF_OpenFont("LiberationMono-Regular.ttf", 14);

	Black.r = 0; Black.g = 0; Black.b = 0;
	White.r = 255; White.g = 255; White.b = 255;
}

		Game::~Game				(void)
{
	GameOptions.SaveOpts();

	// Clean up all of our pointers
	if (SourceDeck)
		delete SourceDeck;
	//if (Window)
	//	SDL_FreeSurface(Window);
	if (DrawFont)
		TTF_CloseFont(DrawFont);
	if (GameOverBig)
		TTF_CloseFont(GameOverBig);
	if (GameOverSmall)
		TTF_CloseFont(GameOverSmall);

	// SDL_ttf cleanup
	if (TTF_WasInit())
		TTF_Quit();
}

bool	Game::OnExecute			(void)
{
	if (!OnInit())
		return false;

	SDL_Event	Event;

	// Main loop
	while (Running)
	{
		while (SDL_PollEvent(&Event))
		{
			OnEvent(&Event);
		}

		// We have to render twice because the computer moves during OnLoop() and we
		// want to render the human's move before the 500ms delay. Hopefully this will
		// be fixed soon.
		OnRender(); 
		OnLoop();
		OnRender();
		SDL_Delay(25);
	}
	
	return true;

}

/* Private methods */

void	Game::AnimatePlay		(Uint8 Index, bool Discard)
{
	bool	CoupFourre	= false;
	Uint8	PileCount	= 0;

	if ((Index < HAND_SIZE) && (Index == FindPopped()) && (IsValidPlay(Index) || Discard))
	{
		Animating = true;

		int StartX, StartY,
			DestX, DestY,
			i = 0, TotalIterations;

		double	IncX, IncY,
				X, Y;

		Uint8	Target = Current,
				Type = CARD_NULL,
				Value = CARD_NULL_NULL;

		Value = Players[Current].GetValue(Index);
		Type = Card::GetTypeFromValue(Value);

		if (Type < CARD_NULL)
		{
			if (Dragging)
			{
				StartX = (DragX - 20) / Dimensions::ScaleFactor;
				StartY = (DragY - 57) / Dimensions::ScaleFactor;
			}
			else
			{
				if (Current == 0)
				{
					GetIndexCoords(Index, StartX, StartY);
				}
				else
				{
					StartX = 140;
					StartY = -61;
				}
			}

			if (Type == CARD_HAZARD)
				Target = 1 - Current;
			else if (Type == CARD_MILEAGE)
				PileCount = Players[Current].GetPileCount(Value);
			else if ((Type == CARD_SAFETY) && (Value - SAFETY_OFFSET == Players[Current].GetQualifiedCoupFourre()))
				CoupFourre = true;

			if (Discard)
			{
				DestX = 3;
				DestY = Dimensions::FirstRowY;
			}
			else
				Tableau::GetTargetCoords(Value, Target, DestX, DestY, CoupFourre, PileCount);

			if (Current == 0)
				FloatSurface.SetImage(Card::GetFileFromValue(Value, CoupFourre));
			else
				FloatSurface.SetImage("gfx/card_bg.png");

			X = StartX;
			Y = StartY;

			if (abs(DestX - X) > abs(DestY - Y))
			{
				IncX = 5;
				TotalIterations = abs(DestX - X) / 5;
				IncY = abs(DestY - Y) / (abs(DestX - X) / 5);
			}
			else
			{
				IncY = 5;
				TotalIterations = abs(DestY - Y) / 5;
				IncX = abs(DestX - X) / (abs(DestY - Y) / 5);
			}

			while ((X != DestX) || (Y != DestY))
			{
				if ((i == TotalIterations / 2) && (Current != 0))
					FloatSurface.SetImage(Card::GetFileFromValue(Value, CoupFourre));

				if (abs(DestX - X) < IncX)
					X = DestX;
				else
				{
					if (X > DestX)
						X -= IncX;
					else
						X += IncX;
				}

				if (abs(DestY - Y) < IncY)
					Y = DestY;
				else
				{
					if (Y > DestY)
						Y -= IncY;
					else
						Y += IncY;
				}

				OnRender(true, false);
				
				FloatSurface.Render(X, Y, Window);
				SDL_Flip(Window);

				++i;

				SDL_Delay(5);
			}
		}

		Animating = false;
	}			
}

void	Game::ChangePlayer		(void)
{
	if (!EndOfGame())
	{
		Current = 1 - Current;
		Players[Current].Draw();
	}
}

bool	Game::CheckForChange	(Uint8 &Old, Uint8 &New)
{
	if (Old != New)
	{
		Old = New;
		return true;
	}

	return false;
}

void	Game::ClearMessage		(void)
{
	Message[0] = '\0';
	MessagedAt = 0;
	MessageSurface.Clear();
	Dirty = true;
}

void	Game::ComputerMove		(void)
{
	bool Played = false;

	for (int i = 0; i < HAND_SIZE; ++i)
	{	
		if (IsValidPlay(i))	// Play the first valid move we find (the computer is currently stupid)
		{
			Pop(i);
			Pop(i);
			Played = true;
			break;
		}
	}

	if (Played == false)
	{
		for (int i = 0; i < HAND_SIZE; ++i)	//No valid moves, discard
		{
			Pop(i);
			if (Discard())
				break;
		}
	}
}

bool	Game::Discard			(void)
{
	Uint8	Value =	CARD_NULL_NULL,
			Index =	FindPopped(); // Find out which card is popped

	if (Index < HAND_SIZE)
	{
		Value	= Players[Current].GetValue(Index);

		if (Value != CARD_NULL_NULL)	// Sanity check
		{
			Players[Current].Detach(Index);
			AnimatePlay(Index, true);
			DiscardTop = Value;	// Put the card on top of the discard pile
			Players[Current].Discard(Index);
		}

		Players[Current].UnPop(Index);
		ChangePlayer();

		//Save after each discard
		Save();

		return true;
	}

	return false;
}

bool	Game::EndOfGame			(void)								const
{
	if (Players[0].IsOutOfCards() && Players[1].IsOutOfCards())	//Both players are out of cards
		return true;

	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		if (Players[i].GetMileage() == ((Extended) ? 1000 : 700))	//A player has completed the trip
			return true;
	}

	return false;
}

Uint8	Game::FindPopped		(void)								const
{
	// Find which card in the hand is "popped"
	for (int i = 0; i < HAND_SIZE; ++i)
	{
		if (Players[Current].IsPopped(i))
			return i;
	}

	return 0xFF;
}

Uint8	Game::GetIndex			(int X, int Y)						const
{
	Uint8	Invalid = 0xFF;

	if ((Y >= Dimensions::FirstRowY) && (Y <= (Dimensions::SecondRowY + 57)))
	{
		Uint8	Add = 0,
				Index = 0;

		if (Y > (Dimensions::SecondRowY + 1))	// Clicked the bottom row. Add 4 to the index
			Add = 4;
		else if (Y > (Dimensions::FirstRowY + 54))	//Clicked in the dead zone
			return Invalid;

		if (X >= 81)	//Clicked within hand
		{
			if (((X - 81) % 65) < 41)	//Click was not in horizontal dead zone
				return (((X - 81) / 65) + Add);
		}
	}

	return Invalid;
}

void	Game::GetIndexCoords	(Uint8 Index, int &X, int &Y)				const
{
	if (Index < 3)
	{
		X = 81 + (65 * (Index + 1));
		Y = Dimensions::FirstRowY;
	}
	else
	{
		X = 81 + (65 * (Index - 3));
		Y = Dimensions::SecondRowY;
	}
}

void	Game::GetScores			(void)
{
	if (EndOfGame())
	{
		for (int i = 0; i < PLAYER_COUNT; ++i)
		{
			int Score = 0,
				PlayerSafetyCount = 0, PlayerCoupFourreCount = 0,
				CategoryIndex = 0;

			/* Points scored by everyone */

			// Distance (1pt per mile travelled)
			Score += Players[i].GetMileage();
			ScoreBreakdown[i][CategoryIndex] = Score;
			++CategoryIndex;
			
			for (int j = 0; j < SAFETY_COUNT; ++j) // Get info about safeties and Coup Fourres
			{
				if (Players[i].HasSafety(j + SAFETY_OFFSET))
				{
					++PlayerSafetyCount;
					if (Players[i].HasCoupFourre(j + SAFETY_OFFSET))
						++PlayerCoupFourreCount;
				}
			}

			// 100pt for each safety, no matter how played
			if (PlayerSafetyCount)
			{
				int SafetyScore = PlayerSafetyCount * 100;
				Score += SafetyScore;
				ScoreBreakdown[i][CategoryIndex] = SafetyScore;
			}
			++CategoryIndex;

			// 300pt bonus for all four safeties
			if (PlayerSafetyCount == 4)
			{
				Score += 300;
				ScoreBreakdown[i][CategoryIndex] = 300;
			}
			++CategoryIndex;

			// 300pt bonus for each Coup Fourre
			if (PlayerCoupFourreCount)
			{
				int CoupFourreScore = PlayerCoupFourreCount * 300;
				Score += CoupFourreScore;
				ScoreBreakdown[i][CategoryIndex] += CoupFourreScore;
			}
			++CategoryIndex;

			/* Points scored only by the player which completed the trip */

			if (Players[i].GetMileage() == ((Extended) ? 1000 : 700))
			{
				// 400pt for completing trip
				Score += 400;
				ScoreBreakdown[i][CategoryIndex] = 400;
				++CategoryIndex;

				// 300pt bonus for delayed action (draw pile exhausted before trip completion)
				if (SourceDeck)
				{
					if (SourceDeck->Empty())
					{
						Score += 300;
						ScoreBreakdown[i][CategoryIndex] = 300;
					}
				}
				++CategoryIndex;

				// 300pt bonus for safe trip (no 200-mile cards)
				if (Players[i].GetPileCount(CARD_MILEAGE_200) == 0)
				{
					Score += 300;
					ScoreBreakdown[i][CategoryIndex] = 300;
				}
				++CategoryIndex;

				// 200pt bonus for completing an extended trip
				if (Extended)
				{
					Score += 200;
					ScoreBreakdown[i][CategoryIndex] = 200;
				}
				++CategoryIndex;

				// 500pt shutout bonus (opponent did not play any mileage cards during the hand)
				if (Players[1 - i].GetMileage() == 0)
				{
					Score += 500;
					ScoreBreakdown[i][CategoryIndex] = 500;
				}
			}
			else
				CategoryIndex += 4;

			++CategoryIndex;

			Scores[i] = Score;
			ScoreBreakdown[i][CategoryIndex] = Score;	//Set subtotal

			++CategoryIndex;

			ScoreBreakdown[i][CategoryIndex] = RunningScores[i];	//Set previous score

			++CategoryIndex;

			ScoreBreakdown[i][CategoryIndex] = RunningScores[i] + Score;	//Set total score
		}
	}
}

bool	Game::InDiscardPile		(int X, int Y)						const
{
	return ((X >= 3) && (X <= 43) && (Y >= Dimensions::FirstRowY) && (Y <= (Dimensions::FirstRowY + 57)));
}

bool	Game::IsValidPlay		(Uint8 Index)						const
{
	if (Index >= HAND_SIZE)
		return false;

	Uint8	Type =	Players[Current].GetType(Index),
			Value =	Players[Current].GetValue(Index);

	if (Type == CARD_NULL)
		// The player tapped an empty slot
		return false;

	if (Type == CARD_MILEAGE)
	{
		Uint8 MileageValue = Card::GetMileValue(Value);

		if (!Players[Current].IsRolling())
			// Must be rolling to play mileage cards
			return false;

		if (Players[Current].IsLimited())
		{
			// Enforce speed limit
			if (MileageValue > 50)
				return false;
		}

		if ((Value == CARD_MILEAGE_200) && (Players[Current].GetPileCount(CARD_MILEAGE_200) > 1))
			// Cannot play more than two 200-mile cards
			return false;

		//Cannot go past trip end, 1000 miles if the trip was extended, otherwise 700
		if ((MileageValue + Players[Current].GetMileage()) > ((Extended) ? 1000 : 700))
			// Cannot go past end of trip
			return false;
		
		return true;
	}
	
	if (Type == CARD_HAZARD)
	{
		if (Players[1 - Current].HasSafety(Card::GetMatchingSafety(Value)))
			// If our opponent has the matching safety, the hazard can't be played
			return false;

		if (Value == CARD_HAZARD_SPEED_LIMIT)
		{
			if (Players[1 - Current].IsLimited())
				// Opponent is already limited
				return false;
		}
		else
		{
			if (!Players[1 - Current].IsRolling())
				// Cannot play other hazards unless opponent is rolling
				return false;
		}

		return true;
	}
	
	if (Type == CARD_REMEDY)
	{
		Uint8 TopCard =		Players[Current].GetTopCard();
		Uint8 TopCardType = Card::GetTypeFromValue(TopCard);
		
		if (Value == CARD_REMEDY_ROLL)
		{
			if ((TopCardType != CARD_REMEDY) && (TopCard != CARD_HAZARD_STOP) && (TopCard != CARD_NULL_NULL) && !Players[Current].HasSafety(Card::GetMatchingSafety(TopCard)))
				/* Are we:	1. Playing on top of a remedy
							2. Playing on top of a "stop" card
							3. Playing on an empty battle pile
							4. Playing on a hazard to which we have the matching safety */
				return false;

			if (Players[Current].IsRolling())
				// We're already rolling
				return false;
		}
		else
		{
			if (TopCardType == CARD_HAZARD)
			{
				if (Players[Current].HasSafety(Card::GetMatchingSafety(TopCard)))
					// Remedy is superfluous; we already have the safety
					return false;
			}

			if (Value == CARD_REMEDY_END_LIMIT)
			{
				if (!Players[Current].IsLimited())
					// We're not limited, so we can't end the limit
					return false;
			}
			else
			{
				if (TopCardType != CARD_HAZARD)
					// Other remedies can only be played on top of hazards
					return false;

				if (TopCard != (Value - 5))
					// The remedy does not match the hazard
					return false;
			}
		}

		return true;
	}

	if (Type == CARD_SAFETY)
		// There are no restrictions on playing safeties.
		return true;

	// Default to false. We should never get here unless something went horribly wrong.
	return false;
}

void	Game::OnClick			(int X, int Y)
{
	static	Uint32	LastClick = 0;

	if (Frozen)		//Don't register clicks when frozen
	{
		if ((SDL_GetTicks() - 1000) > FrozenAt)
		{
			Frozen = false;
			FrozenAt = 0;
		}
		else
			return;
	}

	if (Modal < MODAL_NONE)		// Handle clicks on a modal window
	{
		if (Modal == MODAL_EXTENSION)
		{
			if ((Y >= 251) && (Y <= 276))
			{
				if ((X >= 81) && (X < 151))
				{
					Extended = true;
					Modal = MODAL_NONE;
					Dirty = true;
					ChangePlayer();
				}
				else if ((X > 169) && (X <= 239))
				{
					Extended = false;
					ExtensionDeclined = true;
					Modal = MODAL_NONE;
					Dirty = true;
				}

				Save();
			}
		}
		else if (Modal == MODAL_GAME_MENU)
		{
			if ((X >= 50) && (X <= 270))
			{
				if ((Y >= 120) && (Y <= 400))
				{
					int Index = (Y - 120) / 40;

					if (Index < OPTION_COUNT)	//Clicked an option toggle
					{
						GameOptions.SwitchOpt(Index);
						ShowModal(Modal);
						return;
					}
					else if (Index < (OPTION_COUNT + MENU_ITEM_COUNT)) //Clicked a menu item
					{
						switch (Index - OPTION_COUNT)
						{
						case 0:
							ShowModal(MODAL_NEW_GAME);
							break;
						case 1:
							LastScene = Scene;
							Scene = SCENE_MAIN;
							Modal = MODAL_NONE;
							Dirty = true;
							break;
						}
					}
				}
			}						
			if ((X >= 251) && (X <= 277))
			{
				if ((Y >= 83) && (Y <= 109))	// Clicked the X button
				{
					GameOptions.SaveOpts();
					Modal = MODAL_NONE;
					Dirty = true;
				}
			}
		}
		else if (Modal == MODAL_NEW_GAME)
		{
			if ((X >= 65) && (X <= 255) && (Y >= 220) && (Y <= 265))	// Clicked Cancel
				ShowModal(MODAL_GAME_MENU);
			else if ((X >= 130) && (X <= 190) && (Y >= 281) && (Y <= 311))	//Clicked Confirm
			{
				Reset();
				for (int i = 0; i < PLAYER_COUNT; ++i)
					RunningScores[i] = 0;

				Save();

				Modal = MODAL_NONE;
			}
		}	
		return;
	}

	if (Scene == SCENE_MAIN)	//Main menu
	{
		if ((X >= 45) && (X <= 275))
		{
			if ((Y >= 300) && (Y <= 355))	//Clicked Play
			{
				ShowLoading();
				Reset();
				LastScene = SCENE_MAIN;
				Scene =		SCENE_GAME_PLAY;
				Restore();
			}
			if ((Y >= 370) && (Y <= 415))	//Clicked Learn
			{
				ShowLoading();
				Reset();
				LastScene = SCENE_MAIN;
				Scene =		SCENE_LEARN_1;
			}
		}
		if ((X >= (Dimensions::ScreenWidth - LogoSurface.GetWidth())) && (Y >= (Dimensions::ScreenHeight - LogoSurface.GetHeight())))		//Clicked GPL logo
		{
			LastScene = Scene;
			Scene = SCENE_LEGAL;
			return;
		}
	}

	else if (Scene == SCENE_GAME_PLAY)		//In game play
	{
		if (Current == 0) // Don't respond to clicks unless it's the human's turn
		{
			if (Y < Dimensions::TableauHeight)	//Clicked within opponent's tableau
			{
				if ((Y < 45) && (X < 80))		//Clicked Menu button
					ShowModal(MODAL_GAME_MENU);
				else
				{
					// Play selected card, if it's a hazard
					if (Players[Current].GetType(FindPopped()) == CARD_HAZARD)
						Pop(FindPopped());
				}
			}
			else if ((Y > Dimensions::TableauHeight) && (Y < (Dimensions::TableauHeight * 2)))	//Clicked within own tableau
				Pop(FindPopped());	//Play selected card, if any
			else if ((Y >= Dimensions::FirstRowY) && (Y <= (Dimensions::SecondRowY + 57)))	//Clicked within the two rows at the bottom of the screen
			{
				Uint8 Index = GetIndex(X, Y);
				
				if (Index == 0)
				{
					Players[0].UnPop(FindPopped());
					return;
				}
				else
				{
					Index -= 1;

					if (Index < HAND_SIZE)
						Pop(Index);	//Clicked a card, so pop it
					else
					{
						if (InDiscardPile(X, Y))	//Clicked the discard pile
							Discard();
					}
				}
			}
		}
	}

	else if (Scene == SCENE_GAME_OVER)	//Score screen, start new game on click
	{
		Reset();
		LastScene = Scene;
		Scene = SCENE_GAME_PLAY;
		Save();
	}

	else if (IN_TUTORIAL)	//Tutorial scenes
	{
		if ((Y >= 5) && (Y <= 80))
		{
			if ((X >= 5) && (X <= 80))	//Clicked back arrow
			{
				LastScene = Scene;
				if (Scene == SCENE_LEARN_2)
					ClearMessage();
				if (Scene != SCENE_LEARN_1)
					--Scene;
				else
					Scene = SCENE_MAIN;
			}
			else if ((X >= 245) && (X <= 315))	//Clicked forward arrow
			{
				LastScene = Scene;
				if (Scene != SCENE_LEARN_7)
					++Scene;
				else
				{
					ClearMessage();
					Scene = SCENE_MAIN;
				}
			}
		}
	}

	else if (Scene == SCENE_LEGAL)	//Legal scene, return to main menu on click
	{
		if (LastClick + 500 >= SDL_GetTicks())
		{
			LastScene = Scene;
			Scene = SCENE_MAIN;
		}
		else
		{
			LastClick = SDL_GetTicks();
			Overlay[2].SetText("Double-click to return.", GameOverBig, &White, &Black);
		}
	}
}

void	Game::OnEvent			(SDL_Event * Event)
{
	int	X = 0, Y = 0;

	if (Event != 0)
	{
		if (Event->type == SDL_QUIT)
			Running = false;
		else if (Event->type == SDL_MOUSEBUTTONDOWN)
		{
			if (Event->button.which == 0)
			{
				MouseDown = true;

				DownX = Event->button.x;
				DownY = Event->button.y;

				if (Scene == SCENE_GAME_PLAY)
					DownIndex = GetIndex(DownX / Dimensions::ScaleFactor, DownY / Dimensions::ScaleFactor) - 1;
			}
		}
		else if (Event->type == SDL_MOUSEBUTTONUP)	//Mouse click
		{
			if (Event->button.which == 0)
			{
				double	Scale = Dimensions::ScaleFactor;
				MouseDown = false;

				X = Event->button.x;
				Y = Event->button.y;

				if ((Scene == SCENE_GAME_PLAY) && (Current == 0) && Dragging)
				{

					if (DownIndex < HAND_SIZE)
					{
						if (Y < (Dimensions::EffectiveTableauHeight * 2))
						{
							//AnimatePlay(DownIndex);
							Pop(DownIndex);
						}
						else if (InDiscardPile(X / Scale, Y / Scale))
							Discard();

						Players[0].UnPop(DownIndex);
						DownIndex = 0xFF;

						FloatSurface.Clear();
					}

					Dragging = false;
				}

				if ((abs(DownX - X) > 5) || (abs(DownY - Y) > 5))
					return;

				if (Scale != 1)
				{
					X /= Scale;
					Y /= Scale;
				}

				OnClick(X, Y);
			}
		}
		else if (Event->type == SDL_MOUSEMOTION)
		{
			if (MouseDown)
			{
				int MotionX = Event->motion.xrel;
				int MotionY = Event->motion.yrel;

				if ((Scene == SCENE_LEGAL) || (Scene == SCENE_LEARN_1))
				{
					int CurX = Portal.x;
					int CurY = Portal.y;

					int NewX = CurX - MotionX;
					int NewY = CurY - MotionY;

					int MaxX = Overlay[0].GetWidth() - Dimensions::ScreenWidth;
					int MaxY = Overlay[0].GetHeight() - Dimensions::ScreenHeight;

					if (NewX > MaxX)
						NewX = MaxX;
					if (NewX < 0)
						NewX = 0;

					if (NewY > MaxY)
						NewY = MaxY;
					if (NewY < 0)
						NewY = 0;

					if ((NewX != CurX) || (NewY != CurY))
					{
						Portal.x = NewX;
						Portal.y = NewY;
						Overlay[2].Clear();

						Dirty = true;
					}
				}
				else if ((Scene == SCENE_GAME_PLAY) && (Current == 0))
				{
					if (DownIndex < HAND_SIZE)
					{
						DragX = Event->motion.x;
						DragY = Event->motion.y;

						if (!Dragging)
						{
							if ((abs(DownX - DragX) > 5) || (abs(DownY - DragY) > 5))
							{
								Dragging = true;

								Uint8 Value = Players[Current].GetValue(DownIndex);
								Pop(DownIndex);
								Players[0].Detach(DownIndex);
								FloatSurface.SetImage(Card::GetFileFromValue(Value, ((Value - SAFETY_OFFSET) == Players[Current].GetQualifiedCoupFourre())));
							}
						}
						
						Dirty = true;
					}
				}
			}
		}
		else if (Event->type == SDL_KEYUP)	//Debugging purposes
		{
			return;
		}
	}
}

bool	Game::OnInit			(void)
{
	Dirty = false;

	if (!Window)
	{
		// Set up our display if we haven't already
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
			return false;

		#if defined WEBOS_DEVICE
		if(!(Window = SDL_SetVideoMode(0, 0, 0, SDL_SWSURFACE)))
		#else
		if(!(Window = SDL_SetVideoMode(320, 480, 32, SDL_HWSURFACE | SDL_DOUBLEBUF)))
		#endif
			return false;

		Dimensions::SetDimensions(Window->w, Window->h);

		ResetPortal();

		SDL_WM_SetCaption("SDL Mille", "SDL Mille");
	}

	if (IN_TUTORIAL)
	{
		ArrowSurfaces[0].SetImage("gfx/arrowl.png");
		ArrowSurfaces[1].SetImage("gfx/arrowr.png");
		if (Scene >= SCENE_LEARN_2)
		{
			HandSurface.SetImage("gfx/hand.png");
			ShowMessage(TUTORIAL_TEXT[Scene - SCENE_LEARN_2]);
		}
	}

	if (Message[0] != '\0')
		MessageSurface.SetText(Message, GameOverBig);

	if (Scene == SCENE_MAIN)
	{
		Background.SetImage("gfx/scenes/main.png");
		if (!Background)
			return false;

		if (Dimensions::ScreenHeight > 440)
			LogoSurface.SetImage("gfx/gpl.png");
		else
			LogoSurface.SetImage("gfx/gpl_sideways.png");

		Overlay[0].SetImage("gfx/loading.png");

		return true;
	}

	else if ((Scene == SCENE_GAME_PLAY) || (Scene == SCENE_LEARN_2))
	{
		Background.Clear();

		Overlay[0].SetImage("gfx/overlays/game_play_1.png");

		DiscardSurface.SetImage(Card::GetFileFromValue(DiscardTop));
		TargetSurface.SetImage("gfx/drop_target.png");

		if ((SourceDeck != 0) && (SourceDeck->Empty()))
			DrawCardSurface.SetImage(Card::GetFileFromValue(CARD_NULL_NULL));
		else
			DrawCardSurface.SetImage("gfx/card_bg.png");

		if (DrawFont)
		{
			DrawTextSurface.SetInteger(DeckCount, DrawFont, true, &White);
			if (FindPopped() < HAND_SIZE)
			{
				Uint8	Value = Players[Current].GetValue(FindPopped());
				if (Value < CARD_MILEAGE_25)
					CaptionSurface.SetText(CARD_CAPTIONS[Players[Current].GetValue(FindPopped())], DrawFont);
				else
					CaptionSurface.SetText("MILEAGE", DrawFont);
			}
			else
				CaptionSurface.Clear();
		}

		if (Scene == SCENE_GAME_PLAY)
			MenuSurface.SetImage("gfx/menu.png");
		else
			OrbSurface.SetImage("gfx/orb.png");

		return true;
	}

	else if (Scene == SCENE_LEARN_1)
	{
		ResetPortal();

		Background.SetImage("gfx/scenes/green_bg.png");
		Overlay[0].SetImage("gfx/overlays/learn_1.png");

		if (!Background)
			return false;

		if (GameOverBig)
			Overlay[1].SetText("Drag to scroll.", GameOverBig, &White, &Black);


		return true;
	}

	else if (Scene == SCENE_GAME_OVER)
	{
		Background.SetImage("gfx/scenes/green_bg.png");
		if (HumanWon)
			Overlay[0].SetImage("gfx/overlays/game_over_won.jpg");
		else
			Overlay[0].SetImage("gfx/overlays/game_over.png");

		if (GameOverBig)
		{
			ScoreSurfaces[0][1].Clear();
			ScoreSurfaces[0][2].Clear();

			if (HumanWon)
			{
				ScoreSurfaces[0][1].SetText("Human", GameOverBig, &White, &Black);
				ScoreSurfaces[0][2].SetText("CPU", GameOverBig, &White, &Black);
			}
			else
			{
				ScoreSurfaces[0][1].SetText("Human", GameOverBig);
				ScoreSurfaces[0][2].SetText("CPU", GameOverBig);
			}

			for (int i = 1; i < (SCORE_CATEGORY_COUNT + 1); ++i)
			{
				bool ShowRow = false;

				for (int j = 0; j < SCORE_COLUMN_COUNT; ++j)
				{
					ScoreSurfaces[i][j].Clear();

					if (j == 0)
					{
						if (HumanWon)
						{
							for (int k = 0; k < PLAYER_COUNT; ++k)
							{
								if (ScoreBreakdown[k][i - 1] != 0)
								{
									ShowRow = true;
									break;
								}
							}

							if (ShowRow)
								ScoreSurfaces[i][j].SetText(SCORE_CAT_NAMES[i - 1], GameOverBig, &White, &Black);
						}
						else
						{
							ShowRow = true;
							ScoreSurfaces[i][j].SetText(SCORE_CAT_NAMES[i - 1], GameOverBig);
						}
					}
					else
					{
						int Score = ScoreBreakdown[j - 1][i - 1];

						if (HumanWon)
						{
							if ((Score != 0) && ShowRow)
								ScoreSurfaces[i][j].SetInteger(Score, GameOverBig, (i >= (SCORE_CATEGORY_COUNT - 2)), &White, &Black);
						}
						else
							ScoreSurfaces[i][j].SetInteger(Score, GameOverBig, (i >= (SCORE_CATEGORY_COUNT - 2)));
					}
				}
			}
		}

		return true;
	}

	else if (Scene == SCENE_LEGAL)
	{
		SDL_Color	VersionRed = {230, 0, 11, 0};

		ResetPortal();

		Background.SetImage("gfx/scenes/green_bg.png");
		if (!Background)
			return false;

		Overlay[0].SetImage("gfx/overlays/legal.png");

		if (GameOverBig)
			Overlay[1].SetText("Drag to scroll.", GameOverBig, &White, &Black);

		if (GameOverSmall)
			VersionSurface.SetText(VERSION_TEXT, GameOverSmall, &VersionRed);

		return true;
	}

	return false;
}

void	Game::OnLoop			(void)
{
	if (Message[0] != '\0')	//Clear message if necessary
	{
		if (((SDL_GetTicks() - 4000) > MessagedAt) && !IN_DEMO && (Scene != SCENE_GAME_OVER))
		{
			ClearMessage();
			Dirty = true;
		}
	}

	if (Scene == SCENE_GAME_PLAY)
	{
		if (SourceDeck)
			DeckCount = SourceDeck->CardsLeft();

		if (EndOfGame())
		{
			if (!Extended && !ExtensionDeclined)
			{
				if (Players[0].GetMileage() == 700) //Give player option to extend
				{
					if (Modal != MODAL_EXTENSION)
						ShowModal(MODAL_EXTENSION);
				}
				else if (Players[1].GetMileage() == 700) //Computer randomly decides whether to extend
				{
					srand(time(0));
					if (rand() % 2)
					{
						Extended = true;
						ShowMessage("Computer extends trip");
						ChangePlayer();
					}
					else
						ExtensionDeclined = true;
				}
				else	//Hand ended with trip uncompleted
					ExtensionDeclined = true;

				Save();

				return;
			}

			Frozen = true;	//Freeze to prevent premature closing of score screen
			FrozenAt = SDL_GetTicks();

			GetScores();

			/* Do the captioning here */
			if (ScoreBreakdown[0][SCORE_CATEGORY_COUNT - 1] >= 5000)
			{
				bool Won = true;
				bool Tied = false;

				for (int i = 1; i < PLAYER_COUNT; ++i)
				{
					if (ScoreBreakdown[i][SCORE_CATEGORY_COUNT - 1] >= ScoreBreakdown[0][SCORE_CATEGORY_COUNT - 1])
					{
						if (ScoreBreakdown[i][SCORE_CATEGORY_COUNT - 1] > ScoreBreakdown[0][SCORE_CATEGORY_COUNT - 1])
						{
							Won = false;
							Tied = false;
							break;
						}
						else
							Tied = true;
					}
				}

				if (Tied)
					Overlay[1].SetText("It's a draw! Click to start next game.", GameOverSmall, &White);
				else if (Won)
				{
					Overlay[1].SetText("Congrats! Click to start next game.", GameOverSmall, &White, &Black);
					HumanWon = true;
				}
			}
			else
			{
				bool ComputerWon = false;

				for (int i = 1; i < PLAYER_COUNT; ++i)
				{
					if (ScoreBreakdown[i][SCORE_CATEGORY_COUNT - 1] >= 5000)
					{
						ComputerWon = true;
						break;
					}
				}

				if (ComputerWon)
					Overlay[1].SetText("Aw, shucks! Click to start next game.", GameOverSmall, &White);
				else
					Overlay[1].SetText("Click to start next hand!", GameOverSmall, &White);
			}

			LastScene = Scene;
			Scene = SCENE_GAME_OVER;	//Switch to score screen

			return;
		}

		if (Players[Current].IsOutOfCards())
			ChangePlayer();

		if (Current == 1)
		{
			// Delay for a moment, then the computer makes a move
			SDL_Delay((GameOptions.GetOpt(OPTION_FAST_GAME)) ? 200 : 500);

			ComputerMove();
		}
	}
}

void	Game::OnPlay			(Uint8 Index)
{
	// DiscardedCard places the correct card on top of the discard pile after a Coup Fourre.
	Uint8 DiscardedCard = CARD_NULL_NULL;

	if (IsValidPlay(Index))
	{
		Uint8 Type = Players[Current].GetType(Index);

		if ((Type == CARD_MILEAGE) || (Type == CARD_REMEDY) || (Type == CARD_SAFETY))
			DiscardedCard = Players[Current].OnPlay(Index);
		else
			Players[1 - Current].ReceiveHazard(Players[Current].GetValue(Index));

		// We "discard" after playing, but the card doesn't actually go to the discard pile.
		Players[Current].Discard(Index);

		Dirty = true;	// Graphics will need to be re-drawn now

		if (Type != CARD_SAFETY)
			ChangePlayer();
		else	//Playing a safety gives us another turn.
		{
			if (DiscardedCard != CARD_NULL_NULL)
				// A Coup Fourre bounced a card off the player's tableau. Put it on
				// the discard pile
				DiscardTop = DiscardedCard;

			if (SourceDeck)
				// We immediately draw another card after playing a safety.
				Players[Current].Draw();
		}

		//Save game after every card played
		Save();
	}
}

void	Game::OnRender			(bool Force, bool Flip)
{
	bool	RefreshedSomething =	false, // We only flip the display if something changed
			SceneChanged =			false; // Control variable. Do we need to call OnInit()?

	#ifdef DEBUG
	static	Uint32	LastReset = 0;
	static	Uint32	FrameCount = 0;

	++FrameCount;

	Uint32 TickCount = SDL_GetTicks();

	if ((LastReset + 333) < TickCount)
	{
		LastReset = TickCount;
		DebugSurface.SetInteger(FrameCount * 3, GameOverBig, true, &Black, &White);
		FrameCount = 0;
		RefreshedSomething = true;
	}
	#endif

	if ((Modal == MODAL_NONE) || Force)	//Don't re-render during modal, unless forced
	{
		// If the scene, discard pile, or deck count have changed, we need to do a refresh
		SceneChanged |= CheckForChange(OldDeckCount, DeckCount);
		SceneChanged |= CheckForChange(OldDiscardTop, DiscardTop);
		SceneChanged |= CheckForChange(LastScene, Scene);

		// Also if we're otherwise dirty
		Force |= Dirty;

		//Re-render the background if the hand has changed
		if ((Scene == SCENE_GAME_PLAY) && Players[0].IsDirty())
		{
			if (GameOptions.GetOpt(OPTION_CARD_CAPTIONS)) //With captions, we need to re-init
				SceneChanged = true;
			else	//Without captions, we just need to re-render
				Force = true;
		}

		if (SceneChanged)
			Force = true;

		if ((Scene == SCENE_GAME_PLAY) || IN_DEMO)
		{
			for (int i = (PLAYER_COUNT - 1); i >= 0; --i)
				RefreshedSomething |= Players[i].OnRender(Window, i, Force);
		}


		if (Force)
		{
			if (SceneChanged)
				OnInit(); //Refresh our surfaces

			Force = true;
			RefreshedSomething = true;

		
			// Render the appropriate surfaces
			Background.Render(0, 0, Window);

			if (Scene == SCENE_MAIN)
				LogoSurface.Render(Dimensions::ScreenWidth - LogoSurface.GetWidth(), Dimensions::ScreenHeight - LogoSurface.GetHeight(), Window);
			else if ((Scene == SCENE_GAME_PLAY) || IN_DEMO)
			{
				Overlay[0].Render(0, Dimensions::EffectiveTableauHeight - 1, Window, SCALE_NONE);
				Overlay[0].Render(0, (Dimensions::EffectiveTableauHeight * 2) - 1, Window, SCALE_NONE);

				DiscardSurface.Render(3, Dimensions::FirstRowY, Window);
				DrawCardSurface.Render(3, Dimensions::SecondRowY, Window);
				DrawTextSurface.Render(23 - (DrawTextSurface.GetWidth() / 2), Dimensions::SecondRowY + 33, Window);
			}
			else if (Scene == SCENE_GAME_OVER)
			{
				int X = 0, Y = 0;

				Overlay[0].Render(0, (Dimensions::ScreenHeight - Overlay[0].GetHeight()) / 2, Window);
				Overlay[1].Render((Dimensions::ScreenWidth - Overlay[1].GetWidth()) / 2, Dimensions::ScreenHeight - Overlay[1].GetHeight() - 12, Window);

				for (int i = 0; i < (SCORE_CATEGORY_COUNT + 1); ++i)
				{
					for (int j = 0; j < SCORE_COLUMN_COUNT; ++j)
					{
						if (ScoreSurfaces[i][j])
						{
							int Padding = 25;

							if (Dimensions::ScreenHeight < 480)
								Padding = 10;

							X = 12 + ((j > 0) ? 175 : 0) + ((j > 1) ? 75 : 0);
							Y = Padding + (i * 26) + ((i > 0) ? Padding : 0) + ((i > (SCORE_CATEGORY_COUNT - 3)) ? Padding : 0) + ((i > (SCORE_CATEGORY_COUNT - 1)) ? Padding : 0);
							ScoreSurfaces[i][j].Render(X, Y, Window);
						}
					}
				}
			}
			
			else if ((Scene == SCENE_LEARN_1) || (Scene == SCENE_LEGAL))
			{
				Overlay[0].DrawPart(Portal, Window);

				if ((Portal.x == 0) && (Portal.y == 0) && ((Overlay[0].GetWidth() > Dimensions::ScreenWidth) || (Overlay[0].GetHeight() > Dimensions::ScreenHeight)))
					Overlay[1].Render((Dimensions::ScreenWidth - Overlay[1].GetWidth()) / 2, Dimensions::ScreenHeight - Overlay[1].GetHeight() - 5, Window, SCALE_NONE);

				if (Scene == SCENE_LEGAL)
				{
					if (Portal.y < (VersionSurface.GetHeight() + 1))
						VersionSurface.Render(Dimensions::ScreenWidth - VersionSurface.GetWidth() - 1, 1 - Portal.y, Window, SCALE_NONE);

					Overlay[2].Render((Dimensions::ScreenWidth - Overlay[2].GetWidth()) / 2, 10, Window, SCALE_NONE);
				}
			}
		}

		if ((Scene == SCENE_GAME_PLAY) || IN_DEMO)
		{
			//Render card caption
			if (GameOptions.GetOpt(OPTION_CARD_CAPTIONS))
				CaptionSurface.Render((320 - CaptionSurface.GetWidth()) / 2, ((Dimensions::TableauHeight * 2) - CaptionSurface.GetHeight()) - 10, Window);

			if (Scene == SCENE_GAME_PLAY)
				MenuSurface.Render(2, 5, Window);
		}

		if (IN_TUTORIAL)
		{
			ArrowSurfaces[0].Render(5, 5, Window);	//Render back and forward arrows
			ArrowSurfaces[1].Render(240, 5, Window);

			if (Scene >= SCENE_LEARN_2)
			{
				Uint8 Index = Scene - SCENE_LEARN_2;

				if (Scene < SCENE_LEARN_6)	//Render orb
				{
					if (Index < 2)
						OrbSurface.Render((Dimensions::ScreenWidth - 41) / 2, (Dimensions::TableauHeight * Index) + ((Dimensions::TableauHeight - 41) / 2), Window, SCALE_Y);
					else
						OrbSurface.Render(146, Dimensions::FirstRowY + 8, Window);
				}

				if ((Scene >= SCENE_LEARN_4) && (Scene < SCENE_LEARN_7)) //Render hand icon
				{
					Index -= 2;
					HandSurface.Render(HAND_COORDS[Index], Dimensions::FirstRowY + 32, Window);
				}
			}
		}

		if (MessageSurface)
			MessageSurface.Render((Dimensions::ScreenWidth - MessageSurface.GetWidth()) / 2, Dimensions::TableauHeight - 50, Window, SCALE_Y); //Render the message last.

		if (Dragging && !Animating)
		{
			FloatSurface.Render(DragX - 20, DragY - 67, Window, SCALE_NONE);
		}
	}

	#ifdef DEBUG
	DebugSurface.Render(0, 0, Window);
	#endif

	if (RefreshedSomething && Flip)
		SDL_Flip(Window);
}

void	Game::Pop				(Uint8 Index)
{
	if (Players[Current].IsPopped(Index) && IsValidPlay(Index))
	{
		// If the card is already popped, then play it (if it's a valid play)
		AnimatePlay(Index);
		Players[Current].UnPop(Index);
		OnPlay(Index);
	}
	else
		Players[Current].Pop(Index);	//If it's not already popped, pop it
}

void	Game::Reset				(void)
{
	//Reset my stuff
	if (SourceDeck)
		SourceDeck->Shuffle();
	else
		SourceDeck = new Deck;

	if (SourceDeck)
		OldDeckCount = DeckCount = SourceDeck->CardsLeft();

	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		RunningScores[i] += Scores[i]; //Roll up score
		Scores[i] = 0;
		
		for (int j = 0; j < SCORE_CATEGORY_COUNT; ++j)
			ScoreBreakdown[i][j] = 0;
	}

	//Reset running scores if the round is over
	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		if (RunningScores[i] >= 5000)
		{
			for (int j = 0; j < PLAYER_COUNT; ++j)
				RunningScores[j] = 0;

			break;
		}
	}

	Dirty = true;
	Extended = false;
	ExtensionDeclined = false;
	HumanWon = false;

	Current = 0;

	OldDiscardTop = DiscardTop = CARD_NULL_NULL;

	//Reset down the chain
	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		Players[i].Reset();
		if (SourceDeck)
			Players[i].SetSource(SourceDeck);
	}

	for (int i = 0; i < 13; ++i) //Staggered deal
		Players[i % 2].Draw();

	//Odds and ends
	if (SourceDeck)
		OldDeckCount = DeckCount = SourceDeck->CardsLeft();
}

void	Game::ResetPortal		(void)
{
	Portal.x = 0;
	Portal.y = 0;
	Portal.w = Dimensions::ScreenWidth;
	Portal.h = Dimensions::ScreenHeight;
}

bool	Game::Restore			(void)
{
	using namespace std;

	bool Success = false;
	ifstream SaveFile ("game.sav", ios::in | ios::binary);

	if (SaveFile.is_open())
	{
		SaveFile.seekg(0);

		int SaveVersion = 0;
		SaveFile.read((char *) &SaveVersion, sizeof(int));

		if (SaveVersion == SAVE_FORMAT_VER)
		{
			SaveFile.read((char *) &Current, sizeof(Uint8));
			SaveFile.read((char *) &RunningScores, sizeof(int) * 2);
			SaveFile.read((char *) &DiscardTop, sizeof(Uint8));
			SaveFile.read((char *) &Extended, sizeof(bool));
			SaveFile.read((char *) &ExtensionDeclined, sizeof(bool));

			SourceDeck->Restore(SaveFile);

			for (int i = 0; i < PLAYER_COUNT; ++i)
				Players[i].Restore(SaveFile);
			
			Success = SaveFile.good();
		}
		else
			Success = false;

		SaveFile.close();
	}

	return Success;	
}

bool	Game::Save				(void)
{
	using namespace std;

	bool Success = false;
	ofstream SaveFile ("game.sav", ios::out | ios::binary);

	if (SaveFile.is_open())
	{
		SaveFile.seekp(0);
		SaveFile.write((char *) &SAVE_FORMAT_VER, sizeof(int));
		SaveFile.write((char *) &Current, sizeof(Uint8));
		SaveFile.write((char *) &RunningScores, sizeof(int) * PLAYER_COUNT);
		SaveFile.write((char *) &DiscardTop, sizeof(Uint8));
		SaveFile.write((char *) &Extended, sizeof(bool));
		SaveFile.write((char *) &ExtensionDeclined, sizeof(bool));
		
		SourceDeck->Save(SaveFile);

		for (int i = 0; i < PLAYER_COUNT; ++i)
			Players[i].Save(SaveFile);

		Success = SaveFile.good();
		SaveFile.close();
	}

	return Success;	

}

void	Game::ShowLoading		(void)
{
	Overlay[0].Render((Dimensions::ScreenWidth - Overlay[0].GetWidth()) / 2, (Dimensions::ScreenHeight - Overlay[0].GetHeight()) / 2, Window, SCALE_NONE);
	SDL_Flip(Window);
}

void	Game::ShowMessage		(const char * Msg, bool SetDirty)
{
	if (strlen(Msg) < MESSAGE_SIZE)
	{
		strcpy(Message, Msg);
		MessagedAt = SDL_GetTicks();
		Dirty |= SetDirty;
	}
}

bool	Game::ShowModal			(Uint8 ModalName)
{
	if (ModalName < MODAL_NONE)
	{
		Modal = ModalName;

		Surface::Draw(Window, Surface::Load("gfx/modals/shadow.png"), 0, 0, true);	//Render shadow

		switch(Modal)
		{
		case MODAL_EXTENSION:
			ModalSurface.SetImage("gfx/modals/extension.png");
			ModalSurface.Render(74, 193, Window);
			break;
		case MODAL_GAME_MENU:
			OnRender(true, false); //Re-render the background, but don't flip it.

			ModalSurface.SetImage("gfx/modals/menu_top.png");
			ModalSurface.Render(40, 80, Window);
			for (int i = 0; i < (OPTION_COUNT + MENU_ITEM_COUNT); ++i)	//Render options and other menu items
			{
				if (i < OPTION_COUNT)
				{
					MenuSurfaces[i][0].SetText(OPTION_NAMES[i], GameOverBig, &White);
					MenuSurfaces[i][1].SetText((GameOptions.GetOpt(i)) ? "ON" : "OFF", GameOverBig, &White);
					MenuSurfaces[i][1].Render(240, 120 + (i * 40), Window);
				}
				else
					MenuSurfaces[i][0].SetText(MENU_ITEM_NAMES[i - OPTION_COUNT], GameOverBig, &White);

				MenuSurfaces[i][0].Render(50, 120 + (i * 40), Window);
			}
			break;
		case MODAL_NEW_GAME:
			ModalSurface.SetImage("gfx/modals/quit.png");
			ModalSurface.Render(60, 165, Window);
			break;
		}		

		SDL_Flip(Window);

		return true;
	}

	return false;
}

}
