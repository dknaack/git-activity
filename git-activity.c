#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <git2.h>

static char *month_name[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static int month_first_day[][12] = {
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};
static char *weekday_name[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

static int
is_leap_year(int year)
{
	int result = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
	return result;
}

static int
calculate_weekday(int year, int day)
{
	int weekday = (day
	    + 5 * ((year - 1) % 4)
	    + 4 * ((year - 1) % 100)
	    + 6 * ((year - 1) % 400)) % 7;
	return weekday;
}

int
main(int argc, char *argv[])
{
	int activity[365] = {0};
	int year = 2024;

	// Initialize libgit2
	git_libgit2_init();

	// Open the repository
	git_repository *repo = NULL;
	int error = git_repository_open(&repo, ".");

	if (error != 0) {
		printf("fatal: not a git respository");
		git_libgit2_shutdown();
		return 1;
	}

	// Iterate through commits and print them
	git_revwalk *walker = NULL;
	git_revwalk_new(&walker, repo);
	git_revwalk_push_head(walker);

	git_oid oid;
	while (git_revwalk_next(&oid, walker) == 0) {
		git_commit *commit = NULL;
		git_commit_lookup(&commit, repo, &oid);
		git_time_t time = git_commit_time(commit);
		struct tm *time_info = localtime(&time);
		int commit_day = time_info->tm_yday;
		int commit_year = time_info->tm_year + 1900;
		if (commit_day < 365 && commit_year == year) {
			activity[commit_day]++;
		}

		git_commit_free(commit);
	}

	// Cleanup
	git_revwalk_free(walker);
	git_repository_free(repo);
	git_libgit2_shutdown();

	int max_activity = 0;
	for (int i = 0; i < 365; i++) {
		if (activity[i] > max_activity) {
			max_activity = activity[i];
		}
	}

	printf("    ");
	int leap = is_leap_year(year);
	int days = 0;
	int month = 0;
	for (int week = 0; month < 12 && week < 52; week++) {
		days += 7;

		if (days >= month_first_day[leap][month]) {
			printf("%s ", month_name[month]);
			month++;
			week++;
			days += 7;
		} else {
			printf("  ");
		}
	}
	printf("\n");

	int first_weekday = calculate_weekday(year, 0);
	int last_weekday = calculate_weekday(year, 364);

	for (int weekday = 0; weekday < 7; weekday++) {
		if (weekday % 2 == 1) {
			printf("\x1b[0m%s ", weekday_name[weekday]);
		} else {
			printf("    ");
		}

		for (int week = 0; week < 52; week++) {
			int i = week * 7 + weekday;
			if (week == 0 && weekday < first_weekday) {
				printf("  ");
			} else if (week == 51 && weekday >= last_weekday) {
				printf("  ");
			} else {
				char *color = "\x1b[90m";
				float strength = (float)activity[i] / (float)max_activity;
				if (strength > 0.1) {
					color = "\x1b[92m";
				} else if (strength > 0) {
					color = "\x1b[93m";
				}

				printf("%s▆ ", color);
			}

		}

		printf("\n");
	}

	return 0;
}
