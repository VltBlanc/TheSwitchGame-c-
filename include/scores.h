#ifndef SCORES_H
#define SCORES_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SCORES 100
#define MAX_NAME_LENGTH 32
#define SCORES_TIME_FILE "scores_time.csv"
#define SCORES_POINTS_FILE "scores_points.csv"

typedef struct {
  char playername[MAX_NAME_LENGTH];
  float time;
  int difficulty; // 0=Facile, 1=Moyen, 2=Difficile
  int score;      // Calculated score
} ScoreEntry;

typedef enum { SCORE_BY_TIME, SCORE_BY_POINTS } ScoreType;

typedef struct {
  ScoreEntry entries[MAX_SCORES];
  int count;
} ScoreList;

// Function prototypes
// Function prototypes
ScoreList *load_scores(const char *filename);
void free_scores(ScoreList *scores);
void sort_scores_by_time(ScoreList *scores);
void sort_scores_by_points(ScoreList *scores);
int get_top_scores_by_time(ScoreList *scores, ScoreEntry *top_scores,
                           int max_count);
int get_top_scores_by_points(ScoreList *scores, ScoreEntry *top_scores,
                             int max_count);
void print_scores(ScoreList *scores);
bool save_scores(ScoreList *scores, const char *filename);
int add_score(ScoreList *scores, const char *name, float time, int difficulty,
              ScoreType type);
#endif // SCORES_H