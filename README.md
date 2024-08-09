# wilson
David Wilson Computer Rating System

## Authors

- By David Wilson (UW-Milwaukee '65, '66, UW-Madison '67)
- Source code developed 1994-2000

Modified and shared here:
- By Cody Kirkpatrick
- Contact: codyk@talismanred.com
- Last update: 08/2024

## What is this?

For nearly three decades, Mr. David Wilson produced computer ratings for college football teams using this code. His was one of the first rating systems I ever saw the source code for, and I loved its simplicity. I've been able to adapt it for numerous sports over the years.

David retired completely from Wisconsin a few years ago, and his original website (which was hosted by UW) got scrubbed from the web -- but as a part of college football history, his system deserves to carry on.

## How the Wilson ratings work

Wilson's system does *not* produce predictions, point spreads, etc.; it only serves as a way to rate teams based on games already played. All games are weighted evenly (although his code can be set up to count postseason games twice), and game locations and points scored/allowed do not matter -- the only thing that matters is whether you win or lose, and who the game was against.

## More detailed explanation

David describes it as: "for each game a team plays it gets a Game Performance Rating. This is equal to the opponent's rating plus 100 if the team won or minus 100 if the team lost. The team's Performance Rating is the average of its Game Performance Ratings." You can recalculate any time that another game has been played, whether it's the next day, or the next week.

(I need to add his more complete explanation here, but for now, that will have to do.)

## Input files needed

(all three are available here for your first run)
- names.txt (List of team names, going back to the 1800s)
- games.txt (List of games played)
- conf.txt (Conference affiliations)

## Source code

(all three are available here for your first run)
- rate.c
- school.c
- standing.c

## Output files

### From running "rate"
- byrate1.txt, byrate2.txt, ... (Teams in each division, sorted by rating)
- byrating.txt (All teams rated, sorted by rating)
- byname.txt (All teams rated, listed alphabetically)

### From running "standing"
- standing1.txt, standing2.txt, ... (Conference standings, one file for each division)

## You'll probably get warnings

This source code is almost 25 years old. Modern C compilers probably won't like it. But I've gotten variations of the code to compile and run on Linux, Mac, and Windows. Just might take a little patience.

