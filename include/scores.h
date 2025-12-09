#ifndef SCORES_H
#define SCORES_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_SCORES 100
#define MAX_NAME_LENGTH 32
#define SCORES_FILE "scores.csv"

typedef struct {
  char playername[MAX_NAME_LENGTH];
  int score;
} ScoreEntry;

typedef struct {
  ScoreEntry entries[MAX_SCORES];
  int count;
} ScoreList;

// Function prototypes
ScoreList *load_scores(void);
void free_scores(ScoreList *scores);
void sort_scores(ScoreList *scores);
int get_high_scores(ScoreList *scores, ScoreEntry *top_scores, int max_count);
void print_scores(ScoreList *scores);
bool save_scores(ScoreList *scores);
void add_score(ScoreList *scores, const char *name, int score);

#endif // SCORES_H