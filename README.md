# missile-command-replica
A missile command game replicated using C (An assessed assignment from SCC110 Term 3)

[Demonstration video on YouTube](https://www.youtube.com/watch?v=Y9ji1MEX1L8)

My references to the "original": [Gameplay from YouTube 1](https://www.youtube.com/watch?v=nokIGklnBGY), [Gameplay from YouTube 2](https://www.youtube.com/watch?v=we4lY-GEzMk), [The End screen from YouTube](https://www.youtube.com/watch?v=RZ2Ezkp6TaM).

## Replica (left) VS Original (right)
Start screen:
![Start screen, Original VS Replica](https://raw.githubusercontent.com/Gabrielchihonglee/missile-command-replica/master/media/comparestart.gif)

Prep screen:
![Prep screen, Original VS Replica](https://raw.githubusercontent.com/Gabrielchihonglee/missile-command-replica/master/media/compareprep.gif)

Game screen:
![Game screen, Original VS Replica](https://raw.githubusercontent.com/Gabrielchihonglee/missile-command-replica/master/media/comparegame.gif)

Game screen, featuring the fighter jet:
![Game screen (fighter jet), Original VS Replica](https://raw.githubusercontent.com/Gabrielchihonglee/missile-command-replica/master/media/comparefighterjet.gif)

The End screen:
![The End screen, Original VS Replica](https://raw.githubusercontent.com/Gabrielchihonglee/missile-command-replica/master/media/comparetheend.gif)

Highscore screen:
![Highscore screen, Original VS Replica](https://raw.githubusercontent.com/Gabrielchihonglee/missile-command-replica/master/media/comparehighscore.jpg)

## Score calculation method
### Base scores
Event | Scores gained
----- | -------------
Missile hit | 25
Crazy missile hit (not yet implemented) | 125
Fighter jet / UFO hit | 100
Unused missile | 5 / missile
Survived city | 100 / city
### Score multiplier
Level | Score multiplier
----- | ----------------
1 / 2 | 1x
3 / 4 | 2x
5 / 6 | 3x
7 / 8 | 4x
9 / 10 | 5x
11+ | 6x

Info from: [Strategy Wiki](https://strategywiki.org/wiki/Missile_Command/Walkthrough)

## Todo
- [ ] add "crazy" missiles
- [ ] make the game more alike the original one?
- [ ] maybe add sounds?
