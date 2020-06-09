#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "cJSON.h"


/**
 * Test cJSON with and without strtod hook for number parsing
 */
#pragma GCC diagnostic ignored "-Wcast-qual"
static double previous_cJSON_strtod (const char *nptr, char **endptr) {
    double n = 0;
    double sign = 1;
    double scale = 0;
    int subscale = 0;
    int signsubscale = 1;

    /* Has sign? */
    if (*nptr == '-')
    {
        sign = -1;
        nptr++;
    }
    /* is zero */
    if (*nptr == '0')
    {
        nptr++;
    }
    /* Number? */
    if ((*nptr >= '1') && (*nptr <= '9'))
    {
        do
        {
            n = (n * 10.0) + (*nptr++ - '0');
        }
        while ((*nptr >= '0') && (*nptr<='9'));
    }
    /* Fractional part? */
    if ((*nptr == '.') && (nptr[1] >= '0') && (nptr[1] <= '9'))
    {
        nptr++;
        do
        {
            n = (n  *10.0) + (*nptr++ - '0');
            scale--;
        } while ((*nptr >= '0') && (*nptr <= '9'));
    }
    /* Exponent? */
    if ((*nptr == 'e') || (*nptr == 'E'))
    {
        nptr++;
        /* With sign? */
        if (*nptr == '+')
        {
            nptr++;
        }
        else if (*nptr == '-')
        {
            signsubscale = -1;
            nptr++;
        }
        /* Number? */
        while ((*nptr>='0') && (*nptr<='9'))
        {
            subscale = (subscale * 10) + (*nptr++ - '0');
        }
    }

    /* NB this isn't recommended for production code, but in production
     *    code you can work around it
     */
    *endptr = (char*)(unsigned long)nptr;

    /* number = +/- number.fraction * 10^+/- exponent */
    n = sign * n * pow(10.0, (scale + subscale * signsubscale));

    return n;
}

static cJSON_Hooks normal_strtod = {NULL, NULL, strtod};
static cJSON_Hooks our_strtod = {NULL, NULL, previous_cJSON_strtod};

struct test_case {
	const char* str;
	double val;
};

#define FAIL_AND_RETURN(test,reason,hooks_used,json) do {			\
	printf ("Value: %f - FAIL - %s - %s\n", test.val, reason, hooks_used);	\
	if (json)								\
		cJSON_Delete (json);						\
	return;									\
} while (0)

static void check_value (struct test_case test, cJSON_Hooks* hooks, const char* hooks_used)
{
	const char* const label = "dbl";
	cJSON* json = NULL;
	cJSON* num_field = NULL;
	double d;
	char cjson_str[1000];

	sprintf (cjson_str, "{\"%s\":%s}", label, test.str);

	/* Set the hooks to call the correct function to convert */
	cJSON_InitHooks (hooks);

	json = cJSON_Parse (cjson_str);
	if (!json)
		FAIL_AND_RETURN (test, "parsing JSON", hooks_used, json);

	num_field = cJSON_GetObjectItem (json, label);
	if (!num_field)
		FAIL_AND_RETURN (test, "field not there", hooks_used, json);

	d = cJSON_GetNumberValue (num_field);
	if (d != test.val)
		printf ("Value: %f - FAIL - wrong value (got %1.50f) - %s\n",
			test.val, d, hooks_used);
	else
		printf ("Value: %f - SUCCESS\n", test.val);

	cJSON_Delete (json);
}

static void run_test (struct test_case test) {
	/* Built-in number conversion */
	check_value (test, NULL, "built-in");

	/* Use strtod() - should be the same as previous */
	check_value (test, &normal_strtod, "strtod");

	/* Use our conversion value */
	check_value (test, &our_strtod, "our");
}

int main (int argc, char** argv) {
	struct test_case tests[] = {
		{ "0",           0     },
		{ "0.1",         0.1   },
		{ "-0.1",       -0.1   },
		{ "1e4",     10000     },
		{ "2e-2",        0.02  },
		{ "500.005",   500.005 },
		{ "-500.005", -500.005 }
	};
	unsigned int num_cases = sizeof tests / sizeof (struct test_case);
	unsigned int i;

	for (i = 0; i < num_cases; i++)
		run_test (tests[i]);

	return 0;
}
