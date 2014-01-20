/**
 * Rapid-prototyping protection schemes with IEC 61850
 *
 * Copyright (c) 2014 Steven Blair
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ctypes.h"
#if JSON_INTERFACE == 1

#include "json.h"

Item *getIED(char *iedObjectRef) {
	// check contains at least one IED
	if (dataModelIndex.items == NULL) {
		return NULL;
	}

	int i = 0;
	for (i = 0; i < dataModelIndex.numberOfItems; i++) {
		if (strcmp(iedObjectRef, dataModelIndex.items[i].objectRef) == 0) {
			return &dataModelIndex.items[i];
		}
	}

	return NULL;
}

Item *getLD(char *iedObjectRef, char *objectRef) {
	Item *ied = getIED(iedObjectRef);

	// check IED exists
	if (ied == NULL) {
		return NULL;
	}

	// TODO needs AP name

	// check contains a server
	if (ied->items == NULL) {
		return NULL;
	}

	// check contains at least one LD
	if (ied->items[0].items == NULL) {
		return NULL;
	}

	int i = 0;
	for (i = 0; i < ied->items[0].numberOfItems; i++) {
		if (strcmp(objectRef, ied->items[0].items[i].objectRef) == 0) {
			return &ied->items[0].items[i];
		}
	}

	return NULL;
}

Item *getLN(char *iedObjectRef, char *LDObjectRef, char *objectRef) {
	Item *LD = getLD(iedObjectRef, LDObjectRef);

	if (LD == NULL) {
		return NULL;
	}

	int i = 0;
	for (i = 0; i < LD->numberOfItems; i++) {
		if (strcmp(objectRef, LD->items[i].objectRef) == 0) {
			return &(LD->items[i]);
		}
	}

	return NULL;
}

/**
 * Internal helper function. Finds a sub-item within an item, using the object reference string. Only checks within a single layer.
 */
Item *findSingleItem(Item *item, char *objectRef) {
	if (item == NULL || item->numberOfItems == 0) {
		return NULL;
	}

	int i = 0;
	for (i = 0; i < item->numberOfItems; i++) {
		if (strcmp(objectRef, item->items[i].objectRef) == 0) {
			return &item->items[i];
		}
	}

	return NULL;
}

Item *getItem(Item *ln, int num, ...) {
	if (num < 1) {
		return NULL;
	}

	va_list arguments;
	va_start(arguments, num);

	Item *item = ln;
	int i = 0;

	for (i = 0; i < num; i++) {
		char *ref = va_arg(arguments, char *);
//		printf("  %d: %s\n", i, ref);

		item = findSingleItem(item, ref);

		if (item == NULL) {
			va_end(arguments);
			return NULL;
		}
	}

	va_end(arguments);
	return item;
}

/**
 * Internal helper function to find the first occurrence of char c in string s.
 */
int findCharIndex(char *s, char c) {
	int len = strlen(s);
	int i = 0;

	for (i = 0; i < len; i++) {
		if (s[i] == c) {
			return i;
		}
	}

	return -1;
}

Item *getItemFromPath(char *iedObjectRef, char *objectRefPath) {
	Item *ied = getIED(iedObjectRef);
	int slashIndex = findCharIndex(objectRefPath, '/');

	// TODO ignore a single trailing '/' at end of url: check for no chars after slash?

	// check IED exists
	if (ied == NULL) {
		return NULL;
	}

	// check if entire IED is requested
	if (strlen(objectRefPath) == 0) {
		return ied;
	}

	// check if just LD is specified
	if (slashIndex == -1) {
		Item *ld = getLD(iedObjectRef, objectRefPath);
		if (ld != NULL) {
			return ld;
		}
	}
	// check that LD reference is at least one character long
	else if (slashIndex < 1) {
		return NULL;
	}

	// copy LD reference string, and find item
	char ldRef[slashIndex + 1];	// null-terminated
	lstrcpyn(ldRef, objectRefPath, slashIndex + 1);
	Item *ld = getLD(iedObjectRef, ldRef);

//	printf("ldRef: %s\n", ldRef);
//	fflush(stdout);

	// find all items separated by '.'
	char *path = &objectRefPath[slashIndex + 1];
	int index = -1;
	Item *item = ld;
	while (item != NULL && (index = findCharIndex(path, '.')) > 0) {
		char objectRef[index + 1];	// null-terminated
		lstrcpyn(objectRef, path, index + 1);

//		printf("path: %s\n", path);
//		printf("objectRef: %s\n", objectRef);
//		fflush(stdout);

		item = findSingleItem(item, objectRef);
		path = &path[index + 1];
	}

	// find last item
	item = findSingleItem(item, path);

	return item;
}

int setItem(Item *item, char *input) {
	void *data = item->data;
	int i = 0;
	int len = 0;

	if (item == NULL) {
		return 0;
	}

	switch (item->type) {
		case BASIC_TYPE_CONSTRUCTED:
			// compound data types are not allowed
			return 0;
		case BASIC_TYPE_BOOLEAN:
			if (strcmp(input, "false") == 0) {
				*(CTYPE_BOOLEAN *) data = 0;
				return 1;
			}
			else {
				*(CTYPE_BOOLEAN *) data = 1;
				return 1;
			}
		case BASIC_TYPE_INT8:
			return sscanf(input, "%c", (CTYPE_INT8 *) data);	// TODO may not work correctly on ARM platforms
		case BASIC_TYPE_INT16:
			return sscanf(input, "%hd", ((CTYPE_INT16 *) data));
		case BASIC_TYPE_INT32:
			return sscanf(input, "%d", ((CTYPE_INT32 *) data));
		case BASIC_TYPE_INT64:
			return sscanf(input, "%ld", ((CTYPE_INT64 *) data));
		case BASIC_TYPE_INT8U:
			return sscanf(input, "%cu", ((CTYPE_INT8U *) data));
		case BASIC_TYPE_INT16U:
			return sscanf(input, "%hu", ((CTYPE_INT16U *) data));
		case BASIC_TYPE_INT24U:
			return sscanf(input, "%u", ((CTYPE_INT24U *) data));	// TODO format correctly
		case BASIC_TYPE_INT32U:
			return sscanf(input, "%u", ((CTYPE_INT32U *) data));
		case BASIC_TYPE_FLOAT32:
			return sscanf(input, "%f", ((CTYPE_FLOAT32 *) data));
		case BASIC_TYPE_FLOAT64:
			return sscanf(input, "%lf", ((CTYPE_FLOAT64 *) data));
		case BASIC_TYPE_ENUMERATED:
			return sscanf(input, "%u", ((CTYPE_ENUM *) data));
		case BASIC_TYPE_CODED_ENUM:
			return sscanf(input, "%u", ((CTYPE_ENUM *) data));
		// all string types must be null-terminated in data model
		case BASIC_TYPE_OCTET_STRING:
			len = strlen((const char *) data);
			for (i = 0; i < len; i++) {
				((unsigned char*) data)[i] = input[i];
			}
			return 1;
		case BASIC_TYPE_VISIBLE_STRING:
			return sscanf(input, "\"%s\"", (CTYPE_VISSTRING255) data);
		case BASIC_TYPE_UNICODE_STRING:
			return sscanf(input, "\"%ls\"", (wchar_t *) data);
		case BASIC_TYPE_CURRENCY:
			return sscanf(input, "\"%s\"", (CTYPE_VISSTRING255) data);
		default:
			return 0;
	}
}

int itemToJSON(char *buf, Item *item) {
	void *data = item->data;
	int i = 0;
	int len = 0;

	switch (item->type) {
		case BASIC_TYPE_CONSTRUCTED:
			// compound data types are not allowed
			return 0;
		case BASIC_TYPE_BOOLEAN:
			if (*(CTYPE_BOOLEAN *) data == FALSE) {
				return sprintf(buf, "false");
			}
			else {
				return sprintf(buf, "true");
			}
		case BASIC_TYPE_INT8:
			return sprintf(buf, "%d", *((CTYPE_INT8 *) data));
		case BASIC_TYPE_INT16:
			return sprintf(buf, "%hd", *((CTYPE_INT16 *) data));
		case BASIC_TYPE_INT32:
			return sprintf(buf, "%d", *((CTYPE_INT32 *) data));
		case BASIC_TYPE_INT64:
			return sprintf(buf, "%ld", *((CTYPE_INT64 *) data));
		case BASIC_TYPE_INT8U:
			return sprintf(buf, "%u", *((CTYPE_INT8U *) data));
		case BASIC_TYPE_INT16U:
			return sprintf(buf, "%hu", *((CTYPE_INT16U *) data));
		case BASIC_TYPE_INT24U:
			return sprintf(buf, "%u", *((CTYPE_INT24U *) data));	// TODO format correctly
		case BASIC_TYPE_INT32U:
			return sprintf(buf, "%u", *((CTYPE_INT32U *) data));
		case BASIC_TYPE_FLOAT32:
			return sprintf(buf, "%f", *((CTYPE_FLOAT32 *) data));
		case BASIC_TYPE_FLOAT64:
			return sprintf(buf, "%lf", *((CTYPE_FLOAT64 *) data));
		case BASIC_TYPE_ENUMERATED:
			return sprintf(buf, "%u", *((CTYPE_ENUM *) data));
		case BASIC_TYPE_CODED_ENUM:
			return sprintf(buf, "%u", *((CTYPE_ENUM *) data));
		// all string types must be null-terminated in data model
		case BASIC_TYPE_OCTET_STRING:
			len = strlen((const char *) data);
			for (i = 0; i < len; i++) {
				buf[i] = ((unsigned char *) data)[i];
			}
			return len;
		case BASIC_TYPE_VISIBLE_STRING:
			return sprintf(buf, "\"%s\"", (CTYPE_VISSTRING255) data);
		case BASIC_TYPE_UNICODE_STRING:
			return sprintf(buf, "\"%ls\"", (wchar_t *) data);
		case BASIC_TYPE_CURRENCY:
			return sprintf(buf, "\"%s\"", (CTYPE_VISSTRING255) data);
		default:
			return 0;
	}
}

///**
// * Converts enum of BasicType to printable string literal.
// */
//const char* basicTypeToString(Item *item) {
//	switch (item->type) {
//		case BASIC_TYPE_CONSTRUCTED:
//			// compound data types are not allowed
//			return "CONSTRUCTED";
//		case BASIC_TYPE_BOOLEAN:
//			return "BOOLEAN";
//		case BASIC_TYPE_INT8:
//			return "INT8";
//		case BASIC_TYPE_INT16:
//			return "INT16";
//		case BASIC_TYPE_INT32:
//			return "INT32";
//		case BASIC_TYPE_INT64:
//			return "INT64";
//		case BASIC_TYPE_INT8U:
//			return "INT8U";
//		case BASIC_TYPE_INT16U:
//			return "INT16U";
//		case BASIC_TYPE_INT24U:
//			return "INT24U";
//		case BASIC_TYPE_INT32U:
//			return "INT32U";
//		case BASIC_TYPE_FLOAT32:
//			return "FLOAT32";
//		case BASIC_TYPE_FLOAT64:
//			return "FLOAT64";
//		case BASIC_TYPE_ENUMERATED:
//			return "ENUMERATED";
//		case BASIC_TYPE_CODED_ENUM:
//			return "CODED ENUM";
//		case BASIC_TYPE_OCTET_STRING:
//			return "OCTET STRING";
//		case BASIC_TYPE_VISIBLE_STRING:
//			return "VISIBLE STRING";
//		case BASIC_TYPE_UNICODE_STRING:
//			return "UNICODE STRING";
//		case BASIC_TYPE_CURRENCY:
//			return "CURRENCY";
//		default:
//			return "UNKNOWN TYPE";
//	}
//}

int itemDescriptionTreeToJSON(char *buf, Item *root, unsigned char deep) {
#if JSON_OUTPUT_PRETTIFY == 1
	return itemDescriptionTreeToJSONPretty(buf, root, deep);
#else
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	ACSI_REQUEST_CHECK_LENGTH(len);

	buf[len] = '{';
	len++;

	len += sprintf(&buf[len], "\"name\":\"%s\",\"type\":\"%s\"", item->objectRef, item->typeSCL);

	if (item->lnClass != NULL) {
		len += sprintf(&buf[len], ",\"lnClass\":\"%s\"", item->lnClass);
	}
	if (item->CDC != NULL) {
		len += sprintf(&buf[len], ",\"CDC\":\"%s\"", item->CDC);
	}
	if (item->FC != NULL) {
		len += sprintf(&buf[len], ",\"FC\":\"%s\"", item->FC);
	}
	if (item->dchg != TRIGGER_OPTION_NOT_SPECIFIED) {
		len += sprintf(&buf[len], ",\"dchg\":%s", item->dchg == TRIGGER_OPTION_TRUE ? "true" : "false");
	}
	if (item->qchg != TRIGGER_OPTION_NOT_SPECIFIED) {
		len += sprintf(&buf[len], ",\"qchg\":%s", item->qchg == TRIGGER_OPTION_TRUE ? "true" : "false");
	}

	if (deep > 0) {
		len += sprintf(&buf[len], ",\"items\":[");

		// loop through each sub-item
		for (i = 0; i < item->numberOfItems; i++) {
			len += itemDescriptionTreeToJSON(&buf[len], &item->items[i], TRUE);
			if (i < item->numberOfItems - 1) {
				len += sprintf(&buf[len], ",");
			}
			ACSI_REQUEST_CHECK_LENGTH(len);
		}

		len += sprintf(&buf[len], "]");
	}

	buf[len] = '}';
	len++;

	return len;
#endif
}

/**
 * Internal helper function.
 */
int itemDescriptionTreeToJSONPretty2(char *buf, Item *root, int tab) {
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	ACSI_REQUEST_CHECK_LENGTH2(len);

	len += sprintf(&buf[len], "%*s{", tab, " ");
//	len += sprintf(&buf[len], "\n    %*s\"name\" : \"%s\",\n    %*s\"type\" : \"%s\",\n    %*s\"basictype\" : \"%s\",\n    %*s\"items\" : [", tab, " ", item->objectRef, tab, " ", item->typeSCL, tab, " ", basicTypeToString(item), tab, " ");

	len += sprintf(&buf[len], "\n    %*s\"name\" : \"%s\",\n    %*s\"type\" : \"%s\"", tab, " ", item->objectRef, tab, " ", item->typeSCL);

	if (item->lnClass != NULL) {
		len += sprintf(&buf[len], ",\n    %*s\"lnClass\" : \"%s\"", tab, " ", item->lnClass);
	}
	if (item->CDC != NULL) {
		len += sprintf(&buf[len], ",\n    %*s\"CDC\" : \"%s\"", tab, " ", item->CDC);
	}
	if (item->FC != NULL) {
		len += sprintf(&buf[len], ",\n    %*s\"FC\" : \"%s\"", tab, " ", item->FC);
	}
	if (item->dchg != TRIGGER_OPTION_NOT_SPECIFIED) {
		len += sprintf(&buf[len], ",\n    %*s\"dchg\" : %s", tab, " ", item->dchg == TRIGGER_OPTION_TRUE ? "true" : "false");
	}
	if (item->qchg != TRIGGER_OPTION_NOT_SPECIFIED) {
		len += sprintf(&buf[len], ",\n    %*s\"qchg\" : %s", tab, " ", item->qchg == TRIGGER_OPTION_TRUE ? "true" : "false");
	}

	len += sprintf(&buf[len], ",\n    %*s\"items\" : [", tab, " ");

	// loop through each sub-item
	if (item->type == BASIC_TYPE_CONSTRUCTED && item->numberOfItems > 0) {
		for (i = 0; i < item->numberOfItems; i++) {
			if (i == 0) {
				buf[len] = '\n';
				len++;
			}
			len += itemDescriptionTreeToJSONPretty2(&buf[len], &item->items[i], tab + 8);
			ACSI_REQUEST_CHECK_LENGTH2(len);
			if (i < item->numberOfItems - 1) {
				buf[len] = ',';
				len++;
			}
			buf[len] = '\n';
			len++;
		}
		len += sprintf(&buf[len], "    %*s]\n%*s}", tab, " ", tab, " ");
	}
	else {
		len += sprintf(&buf[len], "]\n%*s}", tab, " ");
	}

	return len;
}

int itemDescriptionTreeToJSONPretty(char *buf, Item *root, unsigned char deep) {
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	ACSI_REQUEST_CHECK_LENGTH(len);

	buf[len] = '{';
	len++;

	len += sprintf(&buf[len], "\n    \"name\" : \"%s\",\n    \"type\" : \"%s\"", item->objectRef, item->typeSCL);

	if (item->lnClass != NULL) {
		len += sprintf(&buf[len], ",\n    \"lnClass\" : \"%s\"", item->lnClass);
	}
	if (item->CDC != NULL) {
		len += sprintf(&buf[len], ",\n    \"CDC\" : \"%s\"", item->CDC);
	}
	if (item->FC != NULL) {
		len += sprintf(&buf[len], ",\n    \"FC\" : \"%s\"", item->FC);
	}
	if (item->dchg != TRIGGER_OPTION_NOT_SPECIFIED) {
		len += sprintf(&buf[len], ",\n    \"dchg\" : %s", item->dchg == TRIGGER_OPTION_TRUE ? "true" : "false");
	}
	if (item->qchg != TRIGGER_OPTION_NOT_SPECIFIED) {
		len += sprintf(&buf[len], ",\n    \"qchg\" : %s", item->qchg == TRIGGER_OPTION_TRUE ? "true" : "false");
	}

	if (deep > 0) {
		len += sprintf(&buf[len], ",\n    \"items\" : [");

		// loop through each sub-item
		if (item->type == BASIC_TYPE_CONSTRUCTED && item->numberOfItems > 0) {
			for (i = 0; i < item->numberOfItems; i++) {
				if (i == 0) {
					buf[len] = '\n';
					len++;
				}
				len += itemDescriptionTreeToJSONPretty2(&buf[len], &item->items[i], 8);
				ACSI_REQUEST_CHECK_LENGTH(len);
				if (i < item->numberOfItems - 1) {
					len += sprintf(&buf[len], ",\n");
				}
			}
			len += sprintf(&buf[len], "\n   ]");
		}
		else {
			len += sprintf(&buf[len], "]");
		}
	}

	buf[len] = '\n';
	len++;
	buf[len] = '}';
	len++;

	return len;
}

/**
 * Internal helper function.
 */
int itemTreeToJSONPretty2(char *buf, Item *root, int tab) {
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	ACSI_REQUEST_CHECK_LENGTH2(len);

	if (item->type == BASIC_TYPE_CONSTRUCTED) {
		len += sprintf(&buf[len], "    %*s\"%s\" : {", tab, " ", item->objectRef);
	}
	else {
		len += sprintf(&buf[len], "    %*s\"%s\" : ", tab, " ", item->objectRef);
	}

	if (item->type == BASIC_TYPE_CONSTRUCTED && item->numberOfItems > 0) {
		len += sprintf(&buf[len], "\n");

		// loop through each sub-item
		for (i = 0; i < item->numberOfItems; i++) {
			len += itemTreeToJSONPretty2(&buf[len], &item->items[i], tab + 4);
			ACSI_REQUEST_CHECK_LENGTH2(len);
			if (i < item->numberOfItems - 1) {
				len += sprintf(&buf[len], ",\n");
			}
		}
	}
	else {
		len += itemToJSON(&buf[len], item);
	}

	if (item->type == BASIC_TYPE_CONSTRUCTED) {
		len += sprintf(&buf[len], "\n    %*s}", tab, " ");
	}

	return len;
}

int itemTreeToJSONPretty(char *buf, Item *root) {
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	ACSI_REQUEST_CHECK_LENGTH(len);

	buf[len] = '{';
	len++;

	if (item->type == BASIC_TYPE_CONSTRUCTED && item->numberOfItems > 0) {
		len += sprintf(&buf[len], "\n    \"%s\" : {\n", item->objectRef);

		// loop through each sub-item
		for (i = 0; i < item->numberOfItems; i++) {
			len += itemTreeToJSONPretty2(&buf[len], &item->items[i], 4);
			ACSI_REQUEST_CHECK_LENGTH(len);
			if (i < item->numberOfItems - 1) {
				len += sprintf(&buf[len], ",\n");
			}
		}

		len += sprintf(&buf[len], "\n    }");
	}
	else {
		len += sprintf(&buf[len], "\n    \"%s\" : ", item->objectRef);
		len += itemToJSON(&buf[len], item);
	}

	buf[len] = '\n';
	len++;
	buf[len] = '}';
	len++;

	return len;
}

/**
 * Internal helper function.
 */
int itemTreeToJSON2(char *buf, Item *root, int tab) {
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	len += sprintf(&buf[len], "\"%s\":", item->objectRef);

	if (item->type == BASIC_TYPE_CONSTRUCTED) {
		buf[len] = '{';
		len++;
	}

	if (item->type == BASIC_TYPE_CONSTRUCTED && item->numberOfItems > 0) {
		// loop through each sub-item
		for (i = 0; i < item->numberOfItems; i++) {
			len += itemTreeToJSON2(&buf[len], &item->items[i], tab + 4);
			if (i < item->numberOfItems - 1) {
				buf[len] = ',';
				len++;
			}
			ACSI_REQUEST_CHECK_LENGTH(len);
		}
	}
	else {
		len += itemToJSON(&buf[len], item);
	}

	if (item->type == BASIC_TYPE_CONSTRUCTED) {
		buf[len] = '}';
		len++;
	}

	return len;
}

int itemTreeToJSON(char *buf, Item *root) {
#if JSON_OUTPUT_PRETTIFY == 1
	return itemTreeToJSONPretty(buf, root);
#else
	int len = 0;
	int i = 0;
	Item *item = root;

	if (item == NULL) {
		return 0;
	}

	ACSI_REQUEST_CHECK_LENGTH(len);

	buf[len] = '{';
	len++;

	if (item->type == BASIC_TYPE_CONSTRUCTED && item->numberOfItems > 0) {
		len += sprintf(&buf[len], "\"%s\":{", item->objectRef);

		// loop through each sub-item
		for (i = 0; i < item->numberOfItems; i++) {
			len += itemTreeToJSON2(&buf[len], &item->items[i], 4);
			ACSI_REQUEST_CHECK_LENGTH2(len);
			if (i < item->numberOfItems - 1) {
				len += sprintf(&buf[len], ",");
			}
		}

		len += sprintf(&buf[len], "}");
	}
	else {
		len += sprintf(&buf[len], "\"%s\":", item->objectRef);
		len += itemToJSON(&buf[len], item);
	}

	buf[len] = '}';
	len++;

	return len;
#endif
}

/**
 * Callback function which handles all HTTP requests.
 */
static int handle_http(struct mg_connection *conn) {
	char *url = (char *) &conn->uri[1];	// exclude the starting '/'
	Item *item;

#ifdef USE_SSL
#if USE_HTTP_AUTH == 1
	static const char *passwords_file = "htpasswd.txt";
	FILE *fp = fopen(passwords_file, "r");

	if (fp == NULL || !mg_authorize_digest(conn, fp)) {
		mg_send_digest_auth_request(conn);
		return 1;
	}

	if (fp != NULL) {
		fclose(fp);
	}
#endif
#endif

	// check for method names in url
	if (strcmp(conn->request_method, "GET") == 0) {
		int len = 0;
		char printBuf[ACSI_RESPONSE_MAX_SIZE];

		if (strncmp(url, ACSI_GET_DEFINITION, strlen(ACSI_GET_DEFINITION)) == 0) {
			item = getItemFromPath(conn->server_param, (char *) &url[strlen(ACSI_GET_DEFINITION) + 1]);
			len = itemDescriptionTreeToJSON(printBuf, item, FALSE);
		}
		else if (strncmp(url, ACSI_GET_DIRECTORY, strlen(ACSI_GET_DIRECTORY)) == 0) {
			item = getItemFromPath(conn->server_param, (char *) &url[strlen(ACSI_GET_DIRECTORY) + 1]);
			len = itemDescriptionTreeToJSON(printBuf, item, TRUE);
		}
		else {
			item = getItemFromPath(conn->server_param, (char *) url);
			len = itemTreeToJSON(printBuf, item);
		}

		if (item != NULL && len == -1) {
			mg_send_status(conn, 500);
			mg_send_data(conn, ACSI_BUFFER_OVERRUN, strlen(ACSI_BUFFER_OVERRUN));
			return 1;
		}
		else if (item != NULL && len > 0) {
			mg_send_data(conn, printBuf, len);
			printf("len: %d\n", len);
			fflush(stdout);
			return 1;
		}
	}
	else if (strcmp(conn->request_method, "POST") == 0) {
		item = getItemFromPath(conn->server_param, (char *) url);
		mg_send_data(conn, ACSI_OK, strlen(ACSI_OK));
		return 1;
	}

//	printf("uri: %s, %s\n", conn->uri, (char *) conn->server_param);
//	fflush(stdout);

//	if (strcmp(&conn->uri[1], "C1/exampleMMXU_1.A.phsC.cVal.mag.f") == 0) {
//		Item *tempItem = getItemFromPath("D1Q1SB4", "C1/exampleMMXU_1.A.phsC.cVal.mag.f");
//		setItem(tempItem, "123.456");
//	}

	mg_send_status(conn, 404);
	mg_send_data(conn, ACSI_NOT_FOUND, strlen(ACSI_NOT_FOUND));
	return 1;
}

/**
 * Internal helper function for processing HTTP events on threads.
 */
static void *serve(void *server) {
  while (1) {
	  mg_poll_server((struct mg_server *) server, WEB_SERVER_SELECT_MAX_TIME);
  }
  return NULL;
}

void start_json_interface() {
	init_webservers(&handle_http, serve);
}




// TODO use mg_connect() instead
// TODO combine into one function
// TODO avoid repeating socket open/close?
// From: https://github.com/cesanta/mongoose/blob/master/unit_test.c
// Connects to host:port, and sends formatted request to it. Returns
// malloc-ed reply and reply length, or NULL on error. Reply contains
// everything including headers, not just the message body.
char *wget(const char *host, int port, int *len, const char *fmt, ...) {
	char buf[2000], *reply = NULL;
	int request_len, reply_size = 0, n, sock = -1;
	struct sockaddr_in sin;
	struct hostent *he = NULL;
	va_list ap;

	if (host != NULL && (he = gethostbyname(host)) != NULL && (sock = socket(PF_INET, SOCK_STREAM, 0)) != -1) {
		sin.sin_family = AF_INET;
		sin.sin_port = htons((uint16_t) port);
		sin.sin_addr = * (struct in_addr *) he->h_addr_list[0];
		if (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) == 0) {
			// Format and send the request.
			va_start(ap, fmt);
			request_len = vsnprintf(buf, sizeof(buf), fmt, ap);
			va_end(ap);
			while (request_len > 0 && (n = send(sock, buf, request_len, 0)) > 0) {
				request_len -= n;
			}
			if (request_len == 0) {
				*len = 0;
				while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
					if (*len + n > reply_size) {
						// Leak possible
						reply = (char *) realloc(reply, reply_size + sizeof(buf));
						reply_size += sizeof(buf);
					}
					if (reply != NULL) {
						memcpy(reply + *len, buf, n);
						*len += n;
					}
				}
			}
			closesocket(sock);
		}
	}

	return reply;
}

char *send_http_request(int port, int *len, char *method, char *url) {
	return wget("localhost", port, len, "%s %s HTTP/1.0\r\n\r\n", method, url);
}


#endif
