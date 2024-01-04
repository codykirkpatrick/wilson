/*   C PROGRAM TO RATE COLLEGE FOOTBALL TEAMS
  
    (c) Copyright 1994-8  David Lee Wilson; 620 Pickford St.; Madison, WI  53711
    This program may be freely distributed and used for non-commercial use.
  
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SCALE 100.0
#define CENTER 500.0  /* average rating */

struct TEAM {
    char *name;             /* team name */
    double rating;        /* team rating */
    double next_rating;   /* will be copied to rating for next iteration */
    double def_rating;    /* default rating */
    short int won, lost, tied;   /* raw team record */
    struct GAME *games;     /* link to games played */
    struct TEAM *link;      /* link to next team */
    struct TEAM *link2;     /* for "pending" list when breaking into groups */
    short int division;     /* set if team in listed division */
} *team_list;

struct GAME {
    double result;     /*  1 for win, 0 for tie, -1 for loss */
    struct TEAM *opponent;
    struct GAME *link;   /* link to next game for same team */
};

struct NAME {
    char *name;         /* possible team name variant */
    char *official;     /* official team name */
    struct NAME *link;  /* link to next possible name */
} *name_list;

char save_line[80];     /* orginal input line for error message */
long int ngames;        /* number of games */
short int nteams;       /* number of teams */
int tie_found = 0;      /* if no ties, don't print tie column */
int all_found = 1;      /* set if all teams found in names.txt */

char* division_name[10];
char  division_divider[10];
char* division_label = "-+ *23\"^&#";

void syntax_error() {
    printf("   Sorry, I don't understand: %s",save_line);
    printf("   (Input line for game number %ld)\n",ngames+1);
    exit(1);
}

char *mallocck(size)  int size; {
    char *result;

    if(size <= 0 || size > 1000) {
        printf("Bad memory allocation size requested: %d\n",size);
        exit(1);
    }
    result = (char*) malloc((long) size);
    if(!result) {printf("***Memory Overflow***\n"); exit(1);}
    return result;
}

void read_names() {
    FILE *namesfp;
    char line[256];        /* input line */
    char *cp, *cp2, *official;
    struct NAME *namep;

    namesfp = fopen("names.txt","r");
    if(!namesfp) {printf("Can't open names.txt\n"); exit(1);}
    printf("Reading names.txt...\n");
     /* read input file */
    fgets(line, sizeof line, namesfp);
    while(!feof(namesfp)) {
        official = 0;
        for(cp = line;cp2 = strchr(cp,'|');cp = cp2+1) {
            *cp2 = 0;
            namep = (struct NAME *) mallocck(sizeof(struct NAME));
            namep->name = mallocck(strlen(cp)+1);
            strcpy(namep->name,cp);
            if(!official) official = namep->name;
            namep->official = official;
            namep->link = name_list;
            name_list = namep;
        }
        fgets(line, sizeof line, namesfp);
    }
    fclose(namesfp);
}

void add_game(teamp, score1, opponentp, score2, increment)
 struct TEAM *teamp, *opponentp;
 short int score1, score2;
 int increment;
{
    struct GAME *gamep, *gamep2;

    gamep = (struct GAME *) mallocck(sizeof(struct GAME));
    if(score1 > score2) {
        gamep->result = 1.0;
        teamp->won += increment;
    }
    else if(score1 == score2) {
        gamep->result = 0;
        teamp->tied += increment;
        tie_found = 1;
    }
    else {
        gamep->result = -1.0;
        teamp->lost += increment;
    }
    gamep->opponent = opponentp;
    gamep->link = 0;
    gamep2 = teamp->games;
    if(gamep2) {
        while(gamep2->link) gamep2 = gamep2->link;
        gamep2->link = gamep;
    }
    else teamp->games = gamep;
}

int mystrcmp(s1,s2) register char *s1, *s2; {  /* case insensative strcmp */
    register char ch1, ch2;

    do {
        while((ch1 = toupper(*s1++)) && (ch1<'A' || ch1>'Z'));
        while((ch2 = toupper(*s2++)) && (ch2<'A' || ch2>'Z'));
    } while(ch1 == ch2 && ch1);
    return (ch1 - ch2);
}

struct TEAM *find_team(name) char *name; {
    struct TEAM *teamp;
    struct NAME *namep;

     /* look up official name */
    for(namep=name_list; namep && mystrcmp(name,namep->name);
     namep=namep->link);
    if(!namep) {    /* new name--not found in names.txt */
        printf("Team name not found in names.txt: %s\n",name);
        all_found = 0;
        namep = (struct NAME *) mallocck(sizeof(struct NAME));
        namep->name = mallocck(strlen(name)+1);
        strcpy(namep->name,name);
        namep->official = namep->name;
        namep->link = name_list;
        name_list = namep;
    }
    name = namep->official;
     /* look for a team by this name */
    for(teamp=team_list; teamp && strcmp(name,teamp->name);
     teamp=teamp->link);
    if(!teamp) {    /* new team */
        teamp = (struct TEAM *) mallocck(sizeof(struct TEAM));
        teamp->name = name;
        teamp->rating = 0;
        teamp->next_rating = 0;
        teamp->def_rating = -1;  
        teamp->won = 0;
        teamp->lost = 0;
        teamp->tied = 0;
        teamp->games = 0;
        teamp->division = 0;
        teamp->link = team_list;
        team_list = teamp;
        nteams++;
    }
    return teamp;
}

void read_games() {
    short int bowl;       /* set for bowl games */
    char *name;           /* start of name of team on input line */
    short int score[2];   /* scores for current game */
    char line[80];        /* input line */
    char ch, ch2;         /* a character from the input line */
    short int which;      /* 0 or 1 for which team in current game */
    short int number;     /* used to gather a score */
    char *cp, *cp2;       /* places in input line */
    struct TEAM *teamp, *pair[2];
    struct NAME *namep;
    FILE *gamesfp;

    gamesfp = fopen("games.txt","r");
    if(!gamesfp) {printf("Can't open games.txt\n"); exit(1);}
    printf("Reading games.txt...\n");
    bowl = 0;
    nteams = 0;
    ngames = 0;
     /* read input file */
    fgets(line, sizeof line, gamesfp);
    while(!feof(gamesfp)) {
        strcpy(save_line,line);
        if(!strncmp(line,"=====",5)) {
            bowl = 1;       /* bowl games count double */
        }
        else {
             /* parse input line */
            cp = line-1;
            for(which=0; which<=1; which++) {
                while((ch = *++cp) && ch <= '*');   /* bypass leading blanks */
                 /* check for a date line or blank line */
                if(!ch) goto skip;
                if(ch >= '0' && ch <= '9') {
                     /* some names start with things like: 20th */
                    cp2 = cp;
                    while(*cp2 >= '0' && *cp2 <= '9') cp2++;
                    if(*cp2 < 'A') {
                        /* bypass date */
                        while(*cp2 > ' ') cp2++;
                        while(*cp2 == ' ') cp2++;
                        if(*cp2 < '1') goto skip;
                        /* date preceded score */
                        cp = cp2;
                    }
                }
                name = cp;      /* start of the name */
continue_name:  do {
                    while((ch = *++cp) > ' ');     /* find ending blank */
                    if(! *cp) goto skip;    /* check for header line */
                    cp2 = cp;            /* possible end-of-name location */
                    while((ch = *++cp) && ch <= ' ');  /* find non-blank */
                    if(!ch) goto skip;
                     /* if it's not a digit, the name continues */
                } while(ch < '0' || ch > '9');
                *cp2 = 0;     /* terminate the team name */
                number = 0;   /* gather up the score */
                do {
                    number = 10*number + ch - '0';
                    ch = *++cp;
                } while(ch >= '0' && ch <= '9');
                if(ch>='n' && ch<='t') {
                    /* oops--ordinal like 1st, 2nd, 3rd, 4th.  name continues */
                    *cp2 = ' ';
                    goto continue_name;
                }
                if(ch == ',') ch = *++cp;
                if(ch > ' ') syntax_error();
                score[which] = number;  /* store the score */
                pair[which] = find_team(name);
            }
            add_game(pair[0], score[0], pair[1], score[1], 1);
            add_game(pair[1], score[1], pair[0], score[0], 1);
            if(bowl) {   /* put game in second time for a bowl game */
                add_game(pair[0], score[0], pair[1], score[1], 0);
                add_game(pair[1], score[1], pair[0], score[0], 0);
            }
            ngames++;
        }
        goto read;
skip:   if(which) syntax_error();
read:   fgets(line, sizeof line, gamesfp);
    }
    fclose(gamesfp);
    printf("%d teams, %ld games\n",nteams,ngames);
     /* The following code was used to help look for errors in the
        names.txt file.  It listed teams that played no games and thus
        may have been a misspelling of another entry.  Or it may be
        a school that is no longer playing football but where the
        entry may still be needed for previous years.  To activate this
        code, comment out the "all_found = 0" statement.  */
    all_found = 0;
    if(all_found && nteams > 675)
     for(namep=name_list; namep; namep=namep->link) 
      if(!strcmp(namep->name,namep->official)) {
        name = namep->official;
         /* look for a team by this name */
        for(teamp=team_list; teamp && strcmp(name,teamp->name);
         teamp=teamp->link);
        if(!teamp) {    /* new team */
            printf("%s had no games\n",name);
        }
    }
}

void read_divisions() {
    FILE *fp;
    char line[256];        /* input line */
    int len;
    char *name;
    struct TEAM *teamp;
    int division = 1;
    double div_def_rate = 0;
    int team_count = 0;
    double atof();

    fp = fopen("conf.txt","r");
    if(!fp) {printf("Can't open conf.txt\n"); exit(1);}
    printf("Reading conf.txt...\n");
     /* read input file */
    fgets(line, sizeof line, fp);
    len = strlen(line);
    while(!feof(fp) && len) {
        line[len-1] = 0;  /* drop final \n */
        if(line[0] == '-' || line[0] == '=') { /* if division name */
            division++;
            division_divider[division] = line[0];
            if(division < 10 && len > 11) {
                division_name[division] = malloc(len-11);
                strcpy(division_name[division],line+11);
            }
            div_def_rate = (atof(line+6)-CENTER)/SCALE;
            team_count = 0;
        }
        else if(line[0] < 'A') {  /* if not conference name */
            name = line;   /* find team name */
            while(*name == ' ') name++;
            teamp = find_team(name);
            teamp->division = division;
             /* set default ratings to division default */
            teamp->def_rating = div_def_rate;
            team_count++;
        }
        fgets(line, sizeof line, fp);
        len = strlen(line);
    }
    fclose(fp);
     /* empty final "division" gives default rating for other divisions */
    if(!team_count && division < 10) division_name[division] = 0;
     /* set default rating for remaining teams */
    for(teamp=team_list; teamp; teamp=teamp->link) {
        if(teamp->def_rating == -1) teamp->def_rating = div_def_rate;
    }
}

void calculate_ratings() {
    register double result;     /* result of current game */
    register struct GAME *gamep;
    register double game_count;
    register double opponent_rating;
    register double new_rating, old_rating;
    register double game_rating;
    long int iteration;
    double sum, last_sum;        /* sum of absolute values of new_rating */
    struct TEAM *teamp;
    double diff1, diff2, ratio;  /* used for convergence acceleration */

     /* set default ratings */
    for(teamp=team_list; teamp; teamp=teamp->link) {
        teamp->next_rating =  teamp->def_rating;
        if(teamp->won + teamp->lost + teamp->tied >= 16)
         printf("%s played %d games\n",teamp->name,
          teamp->won + teamp->lost + teamp->tied);
    }
     /* time to calculate the ratings... */
    iteration = 0;
    printf("Calculating ratings...\n");
    while(1) {
         /* copy the ratings... */
        for(teamp=team_list; teamp; teamp=teamp->link)
         teamp->rating = teamp->next_rating;
         /* recalculate each team's rating */
        for(teamp=team_list; teamp; teamp=teamp->link) {
            game_count = 0;
            new_rating = 0;
            old_rating = teamp->rating;
            for(gamep = teamp->games; gamep; gamep=gamep->link) {
                result = gamep->result;
                opponent_rating =  gamep->opponent->rating;
                game_rating = opponent_rating + result;
                if(game_rating > old_rating ? result >= 0 : result <= 0) {
                    game_count += 1;
                    new_rating += game_rating;
                }
                else {
                    game_count += 0.05;
                    new_rating += 0.05*game_rating;
                }
            }
            if(game_count) {
                new_rating = new_rating/game_count;
                teamp->next_rating = (new_rating + old_rating)/2;
/*printf("%s %f %f\n",teamp->name,old_rating,new_rating);*/
            }
        }
        iteration = iteration + 1;
        if(!(iteration % 500)) {
                 /* calculate sum of absolute value of shifts */
                sum = 0;
                for(teamp=team_list; teamp; teamp=teamp->link) {
                    new_rating = teamp->next_rating - teamp->rating;
                    sum += new_rating >= 0 ? new_rating : -new_rating;
                }
                printf("  Iteration %6ld, Average rating shift =%12.7f\n",
                 iteration, SCALE*sum/nteams);
                 /* check if time to exit */
                if(sum/nteams <= 0.0000001) goto out;
                if(sum >= last_sum && iteration >= 100000) goto out;
                if(iteration >= 500000) goto out;
                last_sum = sum;
        }
    }
out:;
    /* make the average rating zero */
    sum = 0;
    for(teamp=team_list; teamp; teamp=teamp->link) {
        sum += teamp->rating;
    }
    sum /= nteams;
    for(teamp=team_list; teamp; teamp=teamp->link) {
        teamp->rating -= sum;
    }
}

void write_ratings() {
    char name[64];
    struct TEAM *teamp;
    struct GAME *gamep;
    double game_count, new_rating, old_rating, result;
    double weight, product, game_rating, opponent_rating;
    struct GAME *sorted[255];
    int ngames, igame, iresult;
    static char* results[3] = {"LOSS","TIE ","WIN "};
    int postseason;

    while(1) {
        printf(" School name: ");
        if(!fgets(name, sizeof name, stdin)) exit(0);
        if(strlen(name) == 0) exit(0);
        teamp = find_team(name);
        game_count = 0;
        new_rating = 0;
        ngames = 0;
        old_rating = teamp->rating;
        printf("\n\n");
        printf("%-17s         Oppon.    Game\n",teamp->name);
        printf(
     "   Opponent      Result   Rating   Rating   Weight   Product   Effect\n");
        printf(
     "--------------   ------   ------   ------   ------   -------   ------\n");
        for(gamep = teamp->games; gamep; gamep=gamep->link) {
            result = gamep->result;
            opponent_rating =  gamep->opponent->rating;
            game_rating = opponent_rating + result;
            if(game_rating > old_rating ? result >= 0 : result <= 0) {
                weight = 1.0;
            }
            else {
                weight = 0.05;
            }
            for(igame=ngames; igame>0 && sorted[igame-1]->opponent->rating <
             opponent_rating; igame--) {
                sorted[igame] = sorted[igame-1];
            }
            sorted[igame] = gamep;
            if(ngames < 255) ngames++;
            if(gamep->link && gamep->link->opponent == gamep->opponent &&
             gamep->link->result == gamep->result) {
                weight *= 2.0;
                gamep = gamep->link;
            }
            game_count += weight;
            product = weight*game_rating;
            new_rating += product;
            printf("%-17s %s%10.1f%9.1f%8.2f%11.1f%+9.1f\n",
             gamep->opponent->name,
             result > 0 ? "WIN " : result < 0 ? "LOSS" : "TIE ",
             opponent_rating*SCALE+CENTER, game_rating*SCALE+CENTER,
             weight, product*SCALE+weight*CENTER,
             weight*(game_rating-teamp->rating)*SCALE);
        }
        if(game_count) {
            printf(
     "                                            =====    =======\n");
            printf("                                         %8.2f%11.1f\n",
             game_count, new_rating*SCALE+game_count*CENTER);
            printf("%36.1f\n",new_rating*SCALE+game_count*CENTER);
            printf("%17s's rating =  ------ =%7.1f\n",
             teamp->name, teamp->rating*SCALE+CENTER);
            printf("%35.2f\n\n",game_count);
        }
        printf("     Team            Rating      Effect\n");
        printf("   ---------------   ------      ------\n");
        for(iresult = -1; iresult <= 1; iresult++) {
            for(igame=0; igame<ngames; igame++) {
                gamep = sorted[igame];
                result = gamep->result;
                if(result == iresult) {
                    opponent_rating =  gamep->opponent->rating;
                    game_rating = opponent_rating + result;
                    if(game_rating > old_rating ? result >= 0 : result <= 0) {
                        weight = 1.0;
                    }
                    else {
                        weight = 0.05;
                    }
                    postseason = gamep->link && 
                     gamep->link->opponent == gamep->opponent &&
                     gamep->link->result == gamep->result;
                    if(postseason) weight *= 2.0;
                    printf("   %-17s%7.1f %s%+7.1f %s",gamep->opponent->name,
                     opponent_rating*SCALE+CENTER,
                     results[iresult+1],(game_rating-old_rating)*SCALE*weight,
                     division_name[gamep->opponent->division]);
                    if(postseason) printf(" postseason");
                    printf("\n");
                }
            }
            if(!iresult) {
                printf(">> %-17s%7.1f <<\n",teamp->name,
                 teamp->rating*SCALE+CENTER);
            }
        }
        printf("\n\n");
    }
}

int main() {
    read_names();
    read_games();
    read_divisions();
    calculate_ratings();
    write_ratings();
    return 0;
}
