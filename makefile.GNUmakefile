
Game.exe: Player.o Deck.o Card.o Game.cpp
	g++ Player.o Deck.o Card.o Game.cpp -o Game.exe

Player.o: Player.cpp Player.h
	g++ -c Player.cpp

Deck.o: Deck.cpp Deck.h
	g++ -c Deck.cpp

Card.o: Card.cpp Card.h
	g++ -c Card.cpp