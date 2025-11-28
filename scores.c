#include "include/scores.h"

ScoreList *load_scores(void) {
  FILE *file = fopen(SCORES_FILE, "r");
  if (!file) {
    return NULL;
  }

  ScoreList *scores = (ScoreList *)malloc(sizeof(ScoreList));
  if (!scores) {
    fclose(file);
    return NULL;
  }

  scores->count = 0;
  char line[256];

  // Skip header line
  fgets(line, sizeof(line), file);

  while (fgets(line, sizeof(line), file) && scores->count < MAX_SCORES) {
    char *comma = strchr(line, ',');
    if (comma) {
      *comma = '\0';
      strncpy(scores->entries[scores->count].playername, line,
              MAX_NAME_LENGTH - 1);
      scores->entries[scores->count].playername[MAX_NAME_LENGTH - 1] = '\0';

      // Remove trailing newline from score
      char *score_str = comma + 1;
      char *newline = strchr(score_str, '\n');
      if (newline)
        *newline = '\0';

      scores->entries[scores->count].score = atoi(score_str);
      scores->count++;
    }
  }

  fclose(file);
  sort_scores(scores);
  return scores;
}

void free_scores(ScoreList *scores) {
  if (scores) {
    free(scores);
  }
}

int compare_scores(const void *a, const void *b) {
  const ScoreEntry *entry_a = (const ScoreEntry *)a;
  const ScoreEntry *entry_b = (const ScoreEntry *)b;
  return entry_b->score - entry_a->score; // Descending order
}

void sort_scores(ScoreList *scores) {
  if (scores && scores->count > 0) {
    qsort(scores->entries, scores->count, sizeof(ScoreEntry), compare_scores);
  }
}

int get_high_scores(ScoreList *scores, ScoreEntry *top_scores, int max_count) {
  if (!scores || !top_scores || max_count <= 0) {
    return 0;
  }

  int count = (scores->count < max_count) ? scores->count : max_count;
  for (int i = 0; i < count; i++) {
    top_scores[i] = scores->entries[i];
  }

  return count;
}

void print_scores(ScoreList *scores) {
  if (!scores) {
    printf("No scores loaded\n");
    return;
  }

  printf("=== HIGH SCORES ===\n");
  for (int i = 0; i < scores->count && i < 10; i++) {
    printf("%d. %s - %d\n", i + 1, scores->entries[i].playername,
           scores->entries[i].score);
  }
}