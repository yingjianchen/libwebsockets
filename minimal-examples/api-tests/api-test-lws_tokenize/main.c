/*
 * lws-api-test-lws_tokenize
 *
 * Copyright (C) 2018 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates the most minimal http server you can make with lws.
 *
 * To keep it simple, it serves stuff from the subdirectory 
 * "./mount-origin" of the directory it was started in.
 * You can change that by changing mount.origin below.
 */

#include <libwebsockets.h>
#include <string.h>

struct expected {
	lws_tokenize_elem e;
	const char *value;
	int len;
};

struct tests {
	const char *string;
	struct expected *exp;
	int count;
	int flags;
};

struct expected expected1[] = {
			{ LWS_TOKZE_TOKEN,		"protocol-1", 10 },
		{ LWS_TOKZE_DELIMITER, ",", 1},
			{ LWS_TOKZE_TOKEN,		"protocol_2", 10 },
		{ LWS_TOKZE_DELIMITER, ",", 1},
			{ LWS_TOKZE_TOKEN,		"protocol3", 9 },
		{ LWS_TOKZE_ENDED, NULL, 0 },
	},
	expected2[] = {
		{ LWS_TOKZE_TOKEN_NAME_COLON,		"Accept-Language", 15 },
			{ LWS_TOKZE_TOKEN,		"fr-CH", 5 },
		{ LWS_TOKZE_DELIMITER,			",", 1 },
			{ LWS_TOKZE_TOKEN,		"fr", 2 },
			{ LWS_TOKZE_DELIMITER,		";", 1},
			{ LWS_TOKZE_TOKEN_NAME_EQUALS,	"q", 1 },
			{ LWS_TOKZE_FLOAT,		"0.9", 3 },
		{ LWS_TOKZE_DELIMITER,			",", 1 },
			{ LWS_TOKZE_TOKEN,		"en", 2 },
			{ LWS_TOKZE_DELIMITER,		";", 1},
			{ LWS_TOKZE_TOKEN_NAME_EQUALS,	"q", 1 },
			{ LWS_TOKZE_FLOAT,		"0.8", 3 },
		{ LWS_TOKZE_DELIMITER,			",", 1 },
			{ LWS_TOKZE_TOKEN,		"de", 2 },
			{ LWS_TOKZE_DELIMITER,		";", 1},
			{ LWS_TOKZE_TOKEN_NAME_EQUALS,	"q", 1 },
			{ LWS_TOKZE_FLOAT,		"0.7", 3 },
		{ LWS_TOKZE_DELIMITER, ",", 1 },
			{ LWS_TOKZE_DELIMITER,		"*", 1 },
			{ LWS_TOKZE_DELIMITER,		";", 1 },
			{ LWS_TOKZE_TOKEN_NAME_EQUALS,	"q", 1 },
			{ LWS_TOKZE_FLOAT,		"0.5", 3 },
		{ LWS_TOKZE_ENDED, NULL, 0 },
	},
	expected3[] = {
			{ LWS_TOKZE_TOKEN_NAME_EQUALS,	"quoted", 6 },
			{ LWS_TOKZE_QUOTED_STRING,	"things:", 7 },
		{ LWS_TOKZE_DELIMITER,			",", 1 },
			{ LWS_TOKZE_INTEGER,		"1234", 4 },
		{ LWS_TOKZE_ENDED, NULL, 0 },
	},
	expected4[] = {
		{ LWS_TOKZE_ERR_COMMA_LIST,		",", 1 },
	},
	expected5[] = {
			{ LWS_TOKZE_TOKEN,		"brokenlist2", 11 },
		{ LWS_TOKZE_DELIMITER, ",", 1 },
		{ LWS_TOKZE_ERR_COMMA_LIST,		",", 1 },
	},
	expected6[] = {
			{ LWS_TOKZE_TOKEN,		"brokenlist3", 11 },
		{ LWS_TOKZE_DELIMITER, ",", 1 },
		{ LWS_TOKZE_ERR_COMMA_LIST,		",", 1 },

	};

struct tests tests[] = {
	{
		" protocol-1, protocol_2\t,\tprotocol3\n",
		expected1, LWS_ARRAY_SIZE(expected1),
		LWS_TOKENIZE_F_MINUS_NONTERM | LWS_TOKENIZE_F_AGG_COLON
	}, {
		"Accept-Language: fr-CH, fr;q=0.9, en;q=0.8, de;q=0.7, *;q=0.5",
		expected2, LWS_ARRAY_SIZE(expected2),
		LWS_TOKENIZE_F_MINUS_NONTERM | LWS_TOKENIZE_F_AGG_COLON
	}, {
		"quoted = \"things:\", 1234",
		expected3, LWS_ARRAY_SIZE(expected3),
		LWS_TOKENIZE_F_MINUS_NONTERM | LWS_TOKENIZE_F_AGG_COLON
	}, {
		", brokenlist1",
		expected4, LWS_ARRAY_SIZE(expected4),
		LWS_TOKENIZE_F_COMMA_SEP_LIST
	}, {
		"brokenlist2,,",
		expected5, LWS_ARRAY_SIZE(expected5),
		LWS_TOKENIZE_F_COMMA_SEP_LIST
	}, {
		"brokenlist3,",
		expected6, LWS_ARRAY_SIZE(expected6),
		LWS_TOKENIZE_F_COMMA_SEP_LIST
	}
};

static const char *element_names[] = {
	"LWS_TOKZE_ERR_UNTERM_STRING",
	"LWS_TOKZE_ERR_MALFORMED_FLOAT",
	"LWS_TOKZE_ERR_NUM_ON_LHS",
	"LWS_TOKZE_ERR_COMMA_LIST",
	"LWS_TOKZE_ENDED",
	"LWS_TOKZE_DELIMITER",
	"LWS_TOKZE_TOKEN",
	"LWS_TOKZE_INTEGER",
	"LWS_TOKZE_FLOAT",
	"LWS_TOKZE_TOKEN_NAME_EQUALS",
	"LWS_TOKZE_TOKEN_NAME_COLON",
	"LWS_TOKZE_QUOTED_STRING",
};

int main(int argc, const char **argv)
{
	struct lws_tokenize ts;
	lws_tokenize_elem e;
	const char *p;
	int n, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;
	int fail = 0, ok = 0;

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS API selftest: lws_tokenize\n");

	if ((p = lws_cmdline_option(argc, argv, "-s")))
		logs = atoi(p);

	for (n = 0; n < (int)LWS_ARRAY_SIZE(tests); n++) {
		int m = 0, in_fail = fail;
		struct expected *exp = tests[n].exp;

		ts.start = tests[n].string;
		ts.len = strlen(ts.start);
		ts.flags = tests[n].flags;

		do {
			e = lws_tokenize(&ts);

			lwsl_info("{ %s, \"%.*s\", %d }\n",
				  element_names[e + 3], (int)ts.token_len,
				  ts.token, (int)ts.token_len);

			if (m == (int)tests[n].count) {
				fail++;
				break;
			}

			if (e != exp->e) {
				fail++;
				break;
			}

			if (e > 0 &&
			    (ts.token_len != exp->len ||
			     memcmp(exp->value, ts.token, exp->len))) {
				fail++;
				break;
			}

			m++;
			exp++;

		} while (e > 0);

		if (fail == in_fail)
			ok++;
	}

	if (p) {
		ts.start = p;
		ts.len = strlen(p);
		ts.flags = LWS_TOKENIZE_F_MINUS_NONTERM |
			   LWS_TOKENIZE_F_AGG_COLON;

		do {
			e = lws_tokenize(&ts);

			printf("{ %s, \"%.*s\", %d }\n",
				  element_names[e + 3], (int)ts.token_len,
				  ts.token, (int)ts.token_len);

		} while (e > 0);
	}


	lwsl_user("Completed: PASS: %d, FAIL: %d\n", ok, fail);

	return !(ok && !fail);
}
