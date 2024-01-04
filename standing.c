/*   C PROGRAM TO PRODUCE CONFERENCE STANDINGS

    (c) Copyright 1996-2000  David Lee Wilson; 620 Pickford St.; Madison, WI  53711
    This program may be freely distributed and used for non-commercial use.

To compile on my MacBook Air M1 in 2022:
20:21 wilson > cc -Wno-implicit-function-declaration -Wno-return-type -o standing standing.c

  
 */

#include <ctype.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

struct TEAM {
    char *name;             /* team name */
    struct CONF *conf;            /* conference */
    short int won, lost, tied;   /* raw team record */
    short int points, op_points;   /* points scored */
    short int cnfwon, cnflost, cnftied;   /* record within the conference */
    short int cnfpoints, cnfop_points;  /* points within conference */
    float rating;
    struct TEAM *link;        /* link to next team */
    struct TEAM *cnflink;       /* link to next team in the same conference */
} *team_list = NULL;

struct CONF {
    char *name;                /* conference name */
    struct TEAM *teams;        /* link to list of teams in conference */
    struct CONF *link;         /* link to next conference */
};

struct DIV {
    char *name;                 /* division name */
    struct CONF *confs;         /* link to list of conferences in division */
    struct DIV *link;           /* link to next conference */
} *div_list = NULL;

struct NAME {
    char *name;         /* possible team name variant */
    char *official;     /* official team name */
    struct NAME *link;  /* link to next possible name */
} *name_list = NULL;

struct GAME {
    char* name1;        /* name of winner */
    int score1;         /* score of winner */
    char* name2;        /* name of loser */
    int score2;         /* score of loser */
    struct GAME *link;  /* link to next game */
} *game_list = NULL;

char save_line[80];     /* orginal input line for error message */
long int ngames;        /* number of games */
char date[80];          /* a date line from games */
char* ties;             /* set if a tie was found */

void syntax_error() {
    printf("   Sorry, I don't understand: %s",save_line);
    printf("   (Input line for game number %ld)\n",ngames+1);
    exit(1);
}

/*
char *malloc();
*/

char *mallocck(size)  int size; {
    char *result;

    if(size <= 0 || size > 1000) {
        printf("Bad memory allocation size requested: %d\n",size);
        exit(1);
    }
    result = malloc((long) size);
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
    if(!teamp) {
        teamp = (struct TEAM *) mallocck(sizeof(struct TEAM));
        teamp->name = name;
        teamp->conf = 0;
        teamp->won = 0;
        teamp->lost = 0;
        teamp->tied = 0;
        teamp->points = 0;
        teamp->op_points = 0;
        teamp->cnfwon = 0;
        teamp->cnflost = 0;
        teamp->cnftied = 0;
        teamp->cnfpoints = 0;
        teamp->cnfop_points = 0;
        teamp->rating = 500.0;
        teamp->link = team_list;
        team_list = teamp;
        teamp->cnflink = NULL;
    }
    return teamp;
}

void read_conferences() {
    FILE *fp;
    char line[256];        /* input line */
    int len;
    char *name;
    struct TEAM *teamp;
    struct CONF *confp=NULL, **confpp=NULL;
    struct DIV *divp=NULL, **divpp;

    divpp = &div_list;
    fp = fopen("conf.txt","r");
    if(!fp) {printf("Can't open conf.txt\n"); exit(1);}
    printf("Reading conf.txt...\n");
     /* read input file */
    fgets(line, sizeof line, fp);
    len = strlen(line);
    while(!feof(fp) && len) {
        line[len-1] = 0;  /* drop final \n */
        if(line[0] >= 'A' && confpp) {  /* if conference name */
            confp = (struct CONF *) mallocck(sizeof(struct CONF));
            confp->name = mallocck(len);
            strcpy(confp->name,line);
            *confpp = confp;
            confp->link = 0;
            confpp = &(confp->link);
            confp->teams = NULL;
        }
        else if(line[0] == '-' && len > 11) {  /* division name */
            divp = (struct DIV *) mallocck(sizeof(struct DIV));
            divp->name = mallocck(len-11);
            strcpy(divp->name,line+11);
            *divpp = divp;
            divp->link = NULL;
            divpp = &(divp->link);
            divp->confs = NULL;
            confpp = &(divp->confs);
            confp = NULL;
        }
        else if(line[0] == '=' && len > 6) {  /* subdivision name */
        }
        else if(confp) {  /* team name */
            name = line;
            while(*name == ' ') name++;
            teamp = find_team(name);
            if(teamp->conf) {
                printf("%s is in 2 conferences\n",name); 
                exit(1);
            }
            teamp->conf = confp;
            teamp->cnflink = confp->teams;
            confp->teams = teamp;
        }
        fgets(line, sizeof line, fp);
        len = strlen(line);
    }
    fclose(fp);
}

void read_ratings() {
    FILE *fp;
    char string[80],name[18],*cp;
    float rating;
    int i;

    fp = fopen("byname.txt","r");
    if(fp) {
        printf("Reading byname.txt...\n");
        fgets(string,80,fp);
        ties = strchr(string,'T');
        fgets(string,80,fp);
        do {
            fgets(name,18,fp);
            i = fscanf(fp,
             ties ? "%*2d%*3d%*3d%11f\n" : "%*2d%*3d%12f\n" ,&rating);
            if(i == 1) {
                cp = name+16;
                while(*cp == ' ' && cp > name) *cp-- = 0;
                find_team(name)->rating = rating;
            }
        } while(i == 1);
    }
    fclose(fp);
}

void add_game(teamp, score1, opponentp, score2)
 struct TEAM *teamp, *opponentp;
 short int score1, score2;
{
    struct GAME *gamep;
    int increment=0;  /* 1 if teams in same conference */

    teamp->points += score1;
    teamp->op_points += score2;
    if(teamp->conf == opponentp->conf) {
        teamp->cnfpoints += score1;
        teamp->cnfop_points += score2;
        increment = 1;
    }
    if(score1 > score2) {
        teamp->won += 1;
        teamp->cnfwon += increment;
    }
    else if(score1 == score2) {
        teamp->tied += 1;
        teamp->cnftied += increment;
    }
    else {
        teamp->lost += 1;
        teamp->cnflost += increment;
    }
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
    struct TEAM *pair[2];
    FILE *gamesfp,*fp;
    struct GAME *gamep;

    gamesfp = fopen("games.txt","r");
    if(!gamesfp) {printf("Can't open games.txt\n"); exit(1);}
    printf("Reading games.txt...\n");
    fp = fopen("gam_neat.txt","w");
    bowl = 0;
    ngames = 0;
     /* read input file */
    fgets(line, sizeof line, gamesfp);
    while(!feof(gamesfp)) {
        strcpy(save_line,line);
        if(!strncmp(line,"=====",5)) {
            bowl = 1;       /* bowl games count double */
            output_games(fp);
            if(fp) fputs(line,fp);
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
                    if(*cp2 >= 'A') goto gotname;
                    while(*cp2 > ' ') cp2++;
                    ch = *cp2;
                    *cp2 = 0;
                    if(ch && strncmp(date,cp,79)) {
                        strncpy(date,cp,79);
                        output_games(fp);
                        if(fp) fprintf(fp,"                   %s\n",cp);
                    }
                    *cp2 = ch;
                    while(*cp2 == ' ') cp2++;
                    if(*cp2 < '1') goto skip;
                    cp = cp2;   /* score follows date */
                }
gotname:        name = cp;      /* start of the name */
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
            add_game(pair[0], score[0], pair[1], score[1]);
            add_game(pair[1], score[1], pair[0], score[0]);
            ngames++;
            if(fp) {
                gamep = (struct GAME*) mallocck(sizeof(struct GAME));
                 /* put winner first */
                if(score[0] > score[1] || (score[0] == score[1] &&
                 strcmp(pair[0]->name,pair[1]->name) > 0)) {
                    gamep->name1 = pair[0]->name;
                    gamep->score1 = score[0];
                    gamep->name2 = pair[1]->name;
                    gamep->score2 = score[1];
                }
                else {
                    gamep->name1 = pair[1]->name;
                    gamep->score1 = score[1];
                    gamep->name2 = pair[0]->name;
                    gamep->score2 = score[0];
                }
                gamep->link = game_list;
                game_list = gamep;
            }
        }
        goto read;
skip:   if(which) syntax_error();
read:   fgets(line, sizeof line, gamesfp);
    }
    output_games(fp);
    fclose(fp);
    fclose(gamesfp);
    printf("%ld games\n",ngames);
}

output_games(fp)  FILE *fp; {
    struct GAME *gamep, *sortp, **sortpp;

    if(!fp) return;
     /* alpha sort */
    gamep = game_list;
    game_list = 0;
    while(gamep) {
        sortpp = &game_list;
        while((sortp = *sortpp) && strcmp(gamep->name1,sortp->name1) > 0)
            sortpp = &(sortp->link);
        *sortpp = gamep;
        gamep = gamep->link;
        (*sortpp)->link = sortp;
    }
    for(gamep = game_list; gamep; gamep=sortp) {
        fprintf(fp,"%-17s%3d   %-17s%3d\n",
         gamep->name1,gamep->score1,gamep->name2,gamep->score2);
        sortp = gamep->link;
        free(gamep);
    }
    game_list = NULL;
}

write_standings() {
    FILE *fp;
    struct DIV *divp;
    struct CONF *confp;
    struct TEAM *teamp, **sortpp, *sortp;
    int decide1, decide2, compare, count;
    float sum_ratings;
    char filename[32];
    int division_count = 0;

    for(divp=div_list; divp; divp=divp->link) {
        sprintf(filename,"standing%d.txt",++division_count);
        fp = fopen(filename,"w");
        if(!fp) {
            printf("can't open standing%d.txt\n",division_count);
            exit(1);
        }
        printf("Writing standing%d.txt\n",division_count);
        fprintf(fp,"%s Football\n", divp->name);
        fprintf(fp,"Conference Standings as of %s\n",date);
        fprintf(fp,"Perfomance Ratings are from David Wilson's system\n");
        for(confp=divp->confs; confp; confp=confp->link) {
            sum_ratings = 0.0;
            count = 0;
            for(teamp=confp->teams; teamp; teamp=teamp->cnflink) {
                sum_ratings += teamp->rating;
                count++;
            }
            fprintf(fp,"\n\n                    %s\n\n",confp->name);
            fprintf(fp,
             "%4.0f Ave. Rating  /--Conference---\\  /---Over-all----\\ Perf\n",
             sum_ratings/count);
            if(ties) {
                fprintf(fp,
              "                  W  L  T  Pts  Opp  W  L  T  Pts  Opp Rate\n");
                fprintf(fp,
              "                  -  -  -  ---  ---  -  -  -  ---  --- ----\n");
            }
            else {
                fprintf(fp,
              "                   W  L   Pts  Opp    W  L   Pts  Opp  Rate\n");
                fprintf(fp,
              "                   -  -   ---  ---    -  -   ---  ---  ----\n");
            }
             /* sort by confernce standing */
            teamp = confp->teams;
            confp->teams = 0;
            while(teamp) {
                 /* in case of equal ratings, use better */
                 /* raw record as tie breaker */
                sortpp = &(confp->teams);
                while(1) {
                    sortp = *sortpp;
                    if(!sortp) break;
                    decide1 = teamp->cnfwon + teamp->cnflost;
                    decide2 = sortp->cnfwon + sortp->cnflost;
                    if(!decide2) compare = teamp->cnfwon - teamp->cnflost;
                    else if(!decide1) compare = sortp->cnflost - sortp->cnfwon;
                    else compare = teamp->cnfwon*decide2 -
                     sortp->cnfwon*decide1;
                    if(compare == 0) compare = teamp->cnfwon+sortp->cnflost-
                     teamp->cnflost-sortp->cnfwon;
                    if(compare == 0) {
                        decide1 = teamp->won + teamp->lost;
                        decide2 = sortp->won + sortp->lost;
                        if(!decide2) compare = teamp->won - teamp->lost;
                        else if(!decide1) compare = sortp->lost - sortp->won;
                        else compare = teamp->won*decide2 - sortp->won*decide1;
                        if(compare == 0) compare = teamp->won+sortp->lost-
                         teamp->lost-sortp->won;
                        if(compare == 0) compare =
                         teamp->rating - sortp->rating;
                    }
                    if(compare > 0) break;
                    sortpp = &(sortp->cnflink);
                };
                *sortpp = teamp;
                teamp = teamp->cnflink;
                (*sortpp)->cnflink = sortp;
            }
            for(teamp=confp->teams; teamp; teamp=teamp->cnflink) {
                if(ties) fprintf(fp,
                 "%-17s%2d%3d%3d%5d%5d%3d%3d%3d%5d%5d%5.0f\n",
                 teamp->name,teamp->cnfwon,teamp->cnflost,teamp->cnftied,
                 teamp->cnfpoints,teamp->cnfop_points,teamp->won,
                 teamp->lost,teamp->tied,teamp->points,
                 teamp->op_points,teamp->rating);
                else fprintf(fp,
                 "%-17s%3d%3d%6d%5d%5d%3d%6d%5d%6.0f\n",
                 teamp->name,teamp->cnfwon,teamp->cnflost,
                 teamp->cnfpoints,teamp->cnfop_points,teamp->won,
                 teamp->lost,teamp->points,
                 teamp->op_points,teamp->rating);
            }
        }
        fclose(fp);
    }
}

int main() {
    read_names();
    read_conferences();
    read_ratings();
    read_games();
    write_standings();
    return 0;
}
