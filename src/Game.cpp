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

			Game::Game			(void)
{
	Current = 0;	// Set current player to Player 1

	// Set our deck pointer to zero, then try to allocate
	SourceDeck = 0;
	SourceDeck = new Deck;

	// Initialize remaining pointers to zero
	Window = 0;
	Background = 0;
	DiscardSurface = 0;
	DrawCardSurface = 0;
	DrawTextSurface = 0;
	ScoresSurface = OutcomeSurface = 0;
	GameOverFont = DrawFont = 0;

	Scene = SCENE_MAIN;
	LastScene = SCENE_INVALID;

	Dirty = true;
	Extended = false;
	Running = true;

	OldDiscardTop = DiscardTop = CARD_NULL_NULL;

	if (SourceDeck)
		OldDeckCount = DeckCount = SourceDeck->CardsLeft();

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

		for (int j = 0; j < SCORE_CATEGORY_COUNT; ++j)
			ScoreBreakdown[i][j] = 0;
	}

	for (int i = 0; i < (SCORE_CATEGORY_COUNT + 2); ++i)
	{
		for (int j = 0; j < SCORE_COLUMN_COUNT; ++j)
		{
			ScoreSurfaces[i][j] = 0;
		}
	}

	DrawFont = TTF_OpenFont("LiberationMono-Regular.ttf", 16);
	GameOverFont = TTF_OpenFont("LiberationMono-Regular.ttf", 18);
}

			Game::~Game			(void)
{
	// Clean up all of our pointers
	if (SourceDeck)
		delete SourceDeck;
	if (Background)
		SDL_FreeSurface(Background);
	if (Window)
		SDL_FreeSurface(Window);
	if (DiscardSurface)
		SDL_FreeSurface(DiscardSurface);
	if (DrawCardSurface)
		SDL_FreeSurface(DrawCardSurface);
	if (DrawTextSurface)
		SDL_FreeSurface(DrawTextSurface);
	if (OutcomeSurface)
		SDL_FreeSurface(OutcomeSurface);
	if (ScoresSurface)
		SDL_FreeSurface(ScoresSurface);
	if (DrawFont)
		TTF_CloseFont(DrawFont);
	if (GameOverFont)
		TTF_CloseFont(GameOverFont);

	// SDL_ttf cleanup
	if (TTF_WasInit())
		TTF_Quit();
}

bool		Game::IsValidPlay	(Uint8 Index)
{
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
			if (Value > CARD_MILEAGE_50)
				return false;
		}

		if ((Value == CARD_MILEAGE_200) && (Players[Current].Get200Count() > 1))
			// Cannot play more than two 200-mile cards
			return false;

		//Cannot go past trip end, 1000 miles if the trip was extended, otherwise 700

		// TODO: Make MaxMileage a class property, to reduce redundancy
		int MaxMileage = (Extended) ? 1000 : 700;

		if ((MileageValue + Players[Current].GetMileage()) > MaxMileage)
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
				/* Are we:	1. Playing on top of a rememdy
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
					// Other rememdies can only be played on top of hazards
					return false;

				if (TopCard != (Value - 5))
					// The rememedy does not match the hazard
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

void		Game::OnClick		(int X, int Y)
{
	if (Scene == SCENE_MAIN)
	{
		if ((X >= 45) && (X <= 275))
		{
			if ((Y >= 300) && (Y <= 355))
			{
				LastScene = SCENE_MAIN;
				Scene =		SCENE_GAME_PLAY;
			}
			if ((Y >= 370) && (Y <= 415))
			{
				LastScene = SCENE_MAIN;
				Scene =		SCENE_LEARN;
			}
		}
	}

	else if (Scene == SCENE_GAME_PLAY)
	{
		if (Current == 0) // Don't respond to clicks unless it's the human's turn
		{
			// TODO: Give user a way to return to main menu

			if ((Y > 175) && (Y < 350))
				; // TODO: Play popped card by clicking human's tableau
			else if ((Y >= 358) && (Y <= 476))
			{
				Uint8	Add = 0,
						Index = 0;

				if (Y >= 425)
					// Player clicked the bottom row of their hand. Add 4 to the index
					Add = 4;

				else if (Y > 410)
					// We don't need to respond to this click
					return; 

				if (X >= 86)
				{
					// This is a fancy way of figuring out which card the user clicked
					if (((X - 86) % 65) <= 31)
					{
						Index = ((X - 86) / 65) + Add;

						// Index 0 is the empty space above the fourth card
						if (Index > 0)
							// Then we pop it
							Pop(Index - 1);
					}
				}

				else if ((X >= 3) && (X <= 43))
					// User clicked the discard pile
					Discard();
			}
		}
	}

	else if (Scene == SCENE_GAME_OVER)
	{
		//Start new game
		Reset();
		LastScene = Scene;
		Scene = SCENE_GAME_PLAY;
	}


	else if (Scene == SCENE_LEARN)
	{
		// Return to main menu on click
		LastScene = SCENE_LEARN;
		Scene = SCENE_MAIN;
	}
}

void		Game::OnEvent		(SDL_Event * Event)
{
	int	X = 0, Y = 0;
	if (Event)
	{
		if (Event->type == SDL_QUIT)
			Running = false;

		else if (Event->type == SDL_MOUSEBUTTONUP)
		{
			X = Event->button.x;
			Y = Event->button.y;
			#ifdef	ANDROID_DEVICE
			if ((Y < 40) || (Y > 760))
				return;
			else
			{
				X = (X / 1.5);
				Y = ((Y - 40) / 1.5);
			}
			printf("Click converted to coords: %u, %u \n", X, Y);
			#endif
			OnClick(X, Y);
		}
	}
}

bool		Game::OnExecute		(void)
{
	if (!OnInit())
	{
		printf("Init error.\n");
		return false;
	}

	SDL_Event	Event;

	// Main loop
	while (Running)
	{
		//printf("Entered main loop.\n");
		while (SDL_PollEvent(&Event))
		{
			//printf("-Processed event.\n");
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

	printf("Quitting.\n");
	
	return true;

}

bool		Game::OnInit		(void)
{
	if (!Window)
	{
		printf("Trying to init window.\n");
		// Set up our display if we haven't already
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			printf("SDL_Init error.\n");
			return false;
		}

		#if defined WEBOS_DEVICE
			if(!(Window = SDL_SetVideoMode(0, 0, 0, SDL_SWSURFACE)))
		#elif defined ANDROID_DEVICE
			if(!(Window = SDL_SetVideoMode(480, 800, 16, SDL_SWSURFACE)))
		#else
			if(!(Window = SDL_SetVideoMode(320, 480, 32, SDL_HWSURFACE | SDL_DOUBLEBUF)))
		#endif
		{
			printf("Could not set video mode.");
			return false;
		}

		SDL_WM_SetCaption("SDL Mille", "mille.ico");
	}

	// Free all of our pointers

	// TODO: Global function to check and free pointers

	if (Background)
	{
		SDL_FreeSurface(Background);
		Background = 0;
	}

	if (DiscardSurface)
	{
		SDL_FreeSurface(DiscardSurface);
		DiscardSurface = 0;
	}

	if (DrawCardSurface)
	{
		SDL_FreeSurface(DrawCardSurface);
		DrawCardSurface = 0;
	}

	if (DrawTextSurface)
	{
		SDL_FreeSurface(DrawTextSurface);
		DrawTextSurface = 0;
	}

	if (OutcomeSurface)
	{
		SDL_FreeSurface(OutcomeSurface);
		OutcomeSurface = 0;
	}

	if (ScoresSurface)
	{
		SDL_FreeSurface(ScoresSurface);
		ScoresSurface = 0;
	}

	for (int i = 0; i < (SCORE_CATEGORY_COUNT + 2); ++i)
	{
		for (int j = 0; j < SCORE_COLUMN_COUNT; ++j)
		{
			if (ScoreSurfaces[i][j])
			{
				SDL_FreeSurface(ScoreSurfaces[i][j]);
				ScoreSurfaces[i][j] = 0;
			}
		}
	}
	
	printf("Surfaces freed.\n");

	if (Scene == SCENE_MAIN)
	{
		printf("Loading scene MAIN.\n");
		
		Background = Surface::Load("gfx/scenes/main.png");

		if (!Background)
		{
			printf("Could not load background");
			return false;
		}

		printf("Background loaded.\n");
		
		return true;
	}

	else if (Scene == SCENE_GAME_PLAY)
	{
		printf("Loading scene GAME PLAY.\n");
		
		Background = Surface::Load("gfx/scenes/game-play.png");

		DiscardSurface = Card::GetImageFromValue(DiscardTop);

		DrawCardSurface = Card::GetImageFromValue(CARD_NULL_NULL);

		if (DrawFont)
		{
			char	DeckCountText[4];

			// Sanity check on DeckCount and make sure it won't overflow our buffer

			// TODO: use abs() here and in SCENE_GAME_OVER for additional safety
			if ((DeckCount <= DECK_SIZE) && (DeckCount < 1000))
			{
				sprintf(DeckCountText, "%u", DeckCount);
				DrawTextSurface = Surface::RenderText(DeckCountText, DrawFont);
			}
		}

		return true;
	}

	else if (Scene == SCENE_LEARN)
	{
		printf("Loading scene LEARN.\n");
		
		Background = Surface::Load("gfx/scenes/learn.png");

		if (!Background)
			return false;

		return true;
	}

	else if (Scene == SCENE_GAME_OVER)
	{
		printf("Loading scene GAME OVER.\n");
		
		for (int j = 0; j < PLAYER_COUNT; ++j)
		{
			for (int i = 0; i < SCORE_CATEGORY_COUNT; ++i)
			{
				printf("Player %u, category %u, score %u\n", j, i, ScoreBreakdown[j][i]);
			}
		}

		Background = Surface::Load("gfx/scenes/game_over.png");

		if (GameOverFont)
		{
			char TempText[21];

			for (int i = 0; i < (SCORE_CATEGORY_COUNT + 2); ++i)
			{
				for (int j = 0; j < SCORE_COLUMN_COUNT; ++j)
				{
					if (i == 0)
					{
						switch (j)
						{
						case 0:
							TempText[0] = '\0';
							break;
						case 1:
							strcpy(TempText, "Human");
							break;
						case 2:
						default:
							strcpy(TempText, "CPU");
						}
					}
					else if (i < (SCORE_CATEGORY_COUNT + 1))
					{
						if (j == 0)
							strcpy(TempText, SCORE_CAT_NAMES[i - 1]);
						else
						{
							int Score = ScoreBreakdown[j - 1][i - 1];
							if (Score == 0)
								strcpy(TempText, "-");
							else
								sprintf(TempText, "%u", ScoreBreakdown[j - 1][i - 1]);
						}
					}
					else
					{
						switch (j)
						{
						case 0:
							strcpy(TempText, "Total");
							break;
						case 1:
						case 2:
						default:
							sprintf(TempText, "%u", Scores[j - 1]);
						}
					}
					ScoreSurfaces[i][j] = Surface::RenderText(TempText, GameOverFont);
				}
			}
		}

		/*
		if (GameOverFont)
		{
			char ScoresText[13];

			OutcomeSurface = Surface::RenderText((Scores[0] > Scores[1]) ? "You won!" : "The computer won :(", GameOverFont);

			// Make sure we don't overflow our buffer with too many digits
			if ((Scores[0] < 10000) && (Scores[1] < 10000))
			{
				sprintf(ScoresText, "%u to %u", Scores[0], Scores[1]);
				ScoresSurface = Surface::RenderText(ScoresText, GameOverFont);
			}
		}
		*/

		return true;
	}

	printf ("Default false from OnInit().\n");
	
	return false;
}

void		Game::OnLoop		(void)
{
	if (Scene == SCENE_GAME_PLAY)
	{
		if (EndOfGame())
		{
			GetScores();

			LastScene = Scene;
			Scene = SCENE_GAME_OVER;

			return;
		}

		if (Players[Current].IsOutOfCards())
			ChangePlayer();

		if (SourceDeck)
		{
			// "1 - Current" gives us the opponent
			Players[1 - Current].Draw();
			DeckCount = SourceDeck->CardsLeft();
		}

		if (Current == 1)
		{
			// Delay for a moment, then the computer makes a move
			SDL_Delay(500);

			bool Played = false;

			for (int i = 0; i < HAND_SIZE; ++i)
			{	
				// Play the first valid move we find (the computer is currently stupid)
				if (IsValidPlay(i))
				{
					OnPlay(i);
					Played = true;
					break;
				}
			}

			if (Played == false)
			{
				// If the computer didn't find a valid move, it will discard the first
				// non-empty slot in its hand
				for (int i = 0; i < HAND_SIZE; ++i)
				{
					Pop(i);
					if (Discard())
						break;
				}
			}
		}
	}
}

void		Game::OnPlay		(Uint8 Index)
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

		else
		{
			if (DiscardedCard != CARD_NULL_NULL)
				// A Coup Fourre bounced a card off the player's tableau. Put it on
				// the discard pile
				DiscardTop = DiscardedCard;

			if (SourceDeck)
			{
				// We immediately draw another card after playing a safety.
				Players[Current].Draw();
				DeckCount = SourceDeck->CardsLeft();
			}
		}
	}
}

void		Game::OnRender		(void)
{
	bool	RefreshedSomething =	false, // We only flip the display if something changed
			SceneChanged =			false; // Control variable. Do we need to call OnInit()?

	// If the scene, discard pile, or deck count have changed, we need to do a refresh
	if (Scene != LastScene)
	{
		SceneChanged = true;
		LastScene = Scene;
	}
	if (OldDiscardTop != DiscardTop)
	{
		SceneChanged = true;
		OldDiscardTop = DiscardTop;
	}
	if (OldDeckCount != DeckCount)
	{
		SceneChanged = true;
		OldDeckCount = DeckCount;
	}

	// Also if we're otherwise dirty
	if (Dirty)
	{
		printf("We're dirty.\n");
		SceneChanged = true;
		Dirty = false;
	}

	if (SceneChanged)
	{
		OnInit(); //Refresh our surfaces
		RefreshedSomething = true;

		// Render the appropriate surfaces
		if (Background)
		{
			printf("Drew background.\n");
			Surface::Draw(Window, Background, 0, 0);
		}

		if (Scene == SCENE_GAME_PLAY)
		{
			if (DiscardSurface)
				Surface::Draw(Window, DiscardSurface, 3, 358);
			if (DrawCardSurface)
				Surface::Draw(Window, DrawCardSurface, 3, 420);
			if (DrawTextSurface)
				Surface::Draw(Window, DrawTextSurface, 23 - (DrawTextSurface->w / 2), 440);
		}

		else if (Scene == SCENE_GAME_OVER)
		{
			int X = 0, Y = 0;

			for (int i = 0; i < (SCORE_CATEGORY_COUNT + 2); ++i)
			{
				for (int j = 0; j < SCORE_COLUMN_COUNT; ++j)
				{
					if (ScoreSurfaces[i][j])
					{
						X = 12 + ((j > 0) ? 175 : 0) + ((j > 1) ? 75 : 0);
						Y = 50 + (i * 26) + ((i > 0) ? 20 : 0) + ((i > SCORE_CATEGORY_COUNT) ? 20 : 0);
						Surface::Draw(Window, ScoreSurfaces[i][j], X, Y);
					}
				}
			}
		}
	}

	// During play, we also need to render our players
	if (Scene == SCENE_GAME_PLAY)
	{
		// SceneChanged will force the players to re-render if this function re-rendered
		RefreshedSomething |= Players[0].OnRender(Window, 0, SceneChanged);
		RefreshedSomething |= Players[1].OnRender(Window, 1, SceneChanged);
	}

	if (RefreshedSomething)
	{
		printf("Flipped window.");
		SDL_Flip(Window);
	}
}

void			Game::Reset			(void)
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
		Scores[i] = 0;
		
		for (int j = 0; j < SCORE_CATEGORY_COUNT; ++j)
			ScoreBreakdown[i][j] = 0;
	}

	Dirty = true;
	Extended = false;

	Current = 0;

	OldDiscardTop = DiscardTop = CARD_NULL_NULL;

	//Reset down the chain
	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		Players[i].Reset();
		if (SourceDeck)
			Players[i].SetSource(SourceDeck);
	}

	//Odds and ends
	if (SourceDeck)
		OldDeckCount = DeckCount = SourceDeck->CardsLeft();
}




/* Private methods */


inline void		Game::ChangePlayer	(void)
{
	Current = 1 - Current;
}

bool			Game::Discard		(void)
{
	bool Success =	false;

	Uint8	Value =	CARD_NULL_NULL,
			Index =	FindPopped(); // Find out which card is popped

	if (Index < HAND_SIZE)
	{
		Value =		Players[Current].GetValue(Index);
		Success =	Players[Current].Discard(Index);

		if (Success)
		{
			if (Value != CARD_NULL_NULL) // Sanity check
				// Put the card on top of the discard pile
				DiscardTop = Value;

			Players[Current].UnPop(Index);
			ChangePlayer();
		}
	}

	return Success;
}

void			Game::GetScores			(void)
{
	if (EndOfGame())
	{
		for (int i = 0; i < PLAYER_COUNT; ++i)
		{
			int Score = 0,
				PlayerSafetyCount = 0, PlayerCoupFourreCount = 0,
				CategoryIndex = 0,
				TripLength = (Extended) ? 1000 : 700;

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

			if (Players[i].GetMileage() == TripLength)
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
				if (Players[i].Get200Count() == 0)
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

			Scores[i] = Score;
		}
	}
}

bool	Game::EndOfGame		(void)
{
	int EndMileage = (Extended) ? 1000 : 700;

	if (Players[0].IsOutOfCards() && Players[1].IsOutOfCards())
		return true;

	for (int i = 0; i < PLAYER_COUNT; ++i)
	{
		if (Players[i].GetMileage() >= EndMileage)
			// Theoretically, the above would be ==, but I've made it >= for now.
			// After more testing, perhaps I will change it to ==
			return true;
	}

	return false;
}

Uint8	Game::FindPopped	(void)	// Find which card in the hand is "popped"
{
	for (int i = 0; i < HAND_SIZE; ++i)
	{
		if (Players[Current].IsPopped(i))
			return i;
	}

	return 0xFF;
}

bool	Game::Pop	(Uint8 Index)
{

	bool Success = false;

	if (Players[Current].IsPopped(Index) && IsValidPlay(Index))
	{
		// If the card is already popped, then play it (if it's a valid play)
		Players[Current].UnPop(Index);
		OnPlay(Index);
		Success = true;
	}

	else
		// If it's not already popped, pop it
		Success = Players[Current].Pop(Index);

	return Success;
}

}
