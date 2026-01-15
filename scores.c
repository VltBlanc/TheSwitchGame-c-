#include "include/scores.h"

ScoreList *load_scores(const char *filename) {
  ScoreList *scores = (ScoreList *)malloc(sizeof(ScoreList));
  if (!scores) {
    return NULL;
  }
  scores->count = 0;

  FILE *file = fopen(filename, "r");
  if (!file) {
    // File doesn't exist yet, return empty list so we can add to it later
    return scores;
  }

  scores->count = 0;
  char line[256];

  // Skip header line
  fgets(line, sizeof(line), file);

  while (fgets(line, sizeof(line), file) && scores->count < MAX_SCORES) {
    // Format: Player,Time,Difficulty,Score
    char *token = strtok(line, ",");
    if (token) {
      strncpy(scores->entries[scores->count].playername, token,
              MAX_NAME_LENGTH - 1);
      scores->entries[scores->count].playername[MAX_NAME_LENGTH - 1] = '\0';

      token = strtok(NULL, ",");
      if (token)
        scores->entries[scores->count].time = atof(token);

      token = strtok(NULL, ",");
      if (token)
        scores->entries[scores->count].difficulty = atoi(token);
      else
        scores->entries[scores->count].difficulty = 1; // Default Medium

      token = strtok(NULL, "\n"); // Score is last
      if (token)
        scores->entries[scores->count].score = atoi(token);
      else {
        // Recalculate if missing (legacy support)
        int multipliers[] = {1, 3, 5};
        int diff = scores->entries[scores->count].difficulty;
        if (diff < 0)
          diff = 0;
        if (diff > 2)
          diff = 2;
        scores->entries[scores->count].score =
            (int)(scores->entries[scores->count].time * multipliers[diff]);
      }

      scores->count++;
    }
  }

  fclose(file);
  return scores;
}

void free_scores(ScoreList *scores) {
  if (scores) {
    free(scores);
  }
}

int compare_scores_time(const void *a, const void *b) {
  const ScoreEntry *entry_a = (const ScoreEntry *)a;
  const ScoreEntry *entry_b = (const ScoreEntry *)b;
  if (entry_b->time > entry_a->time)
    return 1;
  if (entry_b->time < entry_a->time)
    return -1;
  return 0;
}

int compare_scores_points(const void *a, const void *b) {
  const ScoreEntry *entry_a = (const ScoreEntry *)a;
  const ScoreEntry *entry_b = (const ScoreEntry *)b;
  return (entry_b->score - entry_a->score);
}

void sort_scores_by_time(ScoreList *scores) {
  if (scores && scores->count > 0) {
    qsort(scores->entries, scores->count, sizeof(ScoreEntry),
          compare_scores_time);
  }
}

void sort_scores_by_points(ScoreList *scores) {
  if (scores && scores->count > 0) {
    qsort(scores->entries, scores->count, sizeof(ScoreEntry),
          compare_scores_points);
  }
}

int get_top_scores_by_time(ScoreList *scores, ScoreEntry *top_scores,
                           int max_count) {
  if (!scores || !top_scores || max_count <= 0)
    return 0;

  // Create a temporary copy to sort without messing up the original order (if
  // that matters) or just sort the main list. Let's make a copy to be
  // safe/clean if we want multiple views. Actually, sorting the main list is
  // fine, but we need to support two views. So we should copy the data to a
  // temp array, sort that, and return top N.

  ScoreEntry *temp = malloc(sizeof(ScoreEntry) * scores->count);
  if (!temp)
    return 0;

  memcpy(temp, scores->entries, sizeof(ScoreEntry) * scores->count);
  qsort(temp, scores->count, sizeof(ScoreEntry), compare_scores_time);

  int count = (scores->count < max_count) ? scores->count : max_count;
  for (int i = 0; i < count; i++) {
    top_scores[i] = temp[i];
  }

  free(temp);
  return count;
}

int get_top_scores_by_points(ScoreList *scores, ScoreEntry *top_scores,
                             int max_count) {
  if (!scores || !top_scores || max_count <= 0)
    return 0;

  ScoreEntry *temp = malloc(sizeof(ScoreEntry) * scores->count);
  if (!temp)
    return 0;

  memcpy(temp, scores->entries, sizeof(ScoreEntry) * scores->count);
  qsort(temp, scores->count, sizeof(ScoreEntry), compare_scores_points);

  int count = (scores->count < max_count) ? scores->count : max_count;
  for (int i = 0; i < count; i++) {
    top_scores[i] = temp[i];
  }

  free(temp);
  return count;
}

void print_scores(ScoreList *scores) {
  // Deprecated for debug primarily
  (void)scores;
}

bool save_scores(ScoreList *scores, const char *filename) {
  if (!scores || !filename)
    return false;

  FILE *file = fopen(filename, "w");
  if (!file)
    return false;

  fprintf(file, "Player,Time,Difficulty,Score\n");

  for (int i = 0; i < scores->count; i++) {
    fprintf(file, "%s,%.2f,%d,%d\n", scores->entries[i].playername,
            scores->entries[i].time, scores->entries[i].difficulty,
            scores->entries[i].score);
  }

  fclose(file);
  return true;
}

// Returns rank (1-based), or -1 if not added/ranked
int add_score(ScoreList *scores, const char *name, float time, int difficulty,
              ScoreType type) {
  if (!scores || !name)
    return -1;

  int multipliers[] = {1, 3, 5};
  int diffInfo = difficulty;
  if (diffInfo < 0)
    diffInfo = 0;
  if (diffInfo > 2)
    diffInfo = 2;

  int scoreVal = (int)(time * multipliers[diffInfo]);

  int newIndex = -1;

  if (scores->count < MAX_SCORES) {
    newIndex = scores->count;
    scores->count++;
  } else {
    // Find worst
    int worstIdx = -1;
    if (type == SCORE_BY_TIME) {
      float minTime = 999999999.0f;
      for (int i = 0; i < scores->count; i++) {
        if (scores->entries[i].time < minTime) {
          minTime = scores->entries[i].time;
          worstIdx = i;
        }
      }
      // Only replace if strictly better
      if (worstIdx != -1 && time > minTime)
        newIndex = worstIdx;
    } else {
      int minScore = 2147483647;
      for (int i = 0; i < scores->count; i++) {
        if (scores->entries[i].score < minScore) {
          minScore = scores->entries[i].score;
          worstIdx = i;
        }
      }
      if (worstIdx != -1 && scoreVal > minScore)
        newIndex = worstIdx;
    }
  }

  if (newIndex != -1) {
    strncpy(scores->entries[newIndex].playername, name, MAX_NAME_LENGTH - 1);
    scores->entries[newIndex].playername[MAX_NAME_LENGTH - 1] = '\0';
    scores->entries[newIndex].time = time;
    scores->entries[newIndex].difficulty = difficulty;
    scores->entries[newIndex].score = scoreVal;

    // Sort immediately
    if (type == SCORE_BY_TIME)
      sort_scores_by_time(scores);
    else
      sort_scores_by_points(scores);

    // Find rank of newly added score
    for (int i = 0; i < scores->count; i++) {
      if (strcmp(scores->entries[i].playername, name) == 0 &&
          scores->entries[i].time == time &&
          scores->entries[i].score == scoreVal) {
        return i + 1; // 1-based rank
      }
    }
  }

  return -1;
}