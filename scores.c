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

      scores->entries[scores->count].time = atof(score_str);
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
  // Descending order (Longer time is better)
  if (entry_b->time > entry_a->time)
    return 1;
  if (entry_b->time < entry_a->time)
    return -1;
  return 0;
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

  printf("=== BEST TIMES ===\n");
  for (int i = 0; i < scores->count && i < 10; i++) {
    printf("%d. %s - %.2fs\n", i + 1, scores->entries[i].playername,
           scores->entries[i].time);
  }
}

bool save_scores(ScoreList *scores) {
  if (!scores)
    return false;

  FILE *file = fopen(SCORES_FILE, "w");
  if (!file)
    return false;

  // En-tête
  fprintf(file, "Player,Time\n");

  for (int i = 0; i < scores->count; i++) {
    fprintf(file, "%s,%.2f\n", scores->entries[i].playername,
            scores->entries[i].time);
  }

  fclose(file);
  return true;
}

void add_score(ScoreList *scores, const char *name, float time) {
  if (!scores || !name)
    return;

  // Si on n'a pas atteint le max, on ajoute à la fin
  if (scores->count < MAX_SCORES) {
    strncpy(scores->entries[scores->count].playername, name,
            MAX_NAME_LENGTH - 1);
    scores->entries[scores->count].playername[MAX_NAME_LENGTH - 1] = '\0';
    scores->entries[scores->count].time = time;
    scores->count++;
  } else {
    // Si la liste est pleine, on vérifie si le nouveau score est meilleur que
    // le pire (le dernier car trié décroissant)
    if (time > scores->entries[MAX_SCORES - 1].time) {
      strncpy(scores->entries[MAX_SCORES - 1].playername, name,
              MAX_NAME_LENGTH - 1);
      scores->entries[MAX_SCORES - 1].playername[MAX_NAME_LENGTH - 1] = '\0';
      scores->entries[MAX_SCORES - 1].time = time;
    } else {
      // Temps trop court pour entrer dans le top
      return;
    }
  }

  // On re-trie la liste
  sort_scores(scores);
}