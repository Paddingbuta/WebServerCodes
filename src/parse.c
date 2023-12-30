#include "parse.h"
#define BUF_SIZE 4096
/**
 * Given a char buffer returns the parsed request headers
 */
Request *parse(char *buffer, int size, int socketFd, int *tag)
{
	// Differant states in the state machine

	enum
	{
		STATE_START = 0,
		STATE_CR,
		STATE_CRLF,
		STATE_CRLFCR,
		STATE_CRLFCRLF
	};

	int i, state;
	i = *tag;

	size_t offset = 0;
	char ch;
	char buf[20 * BUF_SIZE];
	memset(buf, 0, 20 * BUF_SIZE);

	state = STATE_START;

	while (state != STATE_CRLFCRLF)
	{
		char expected = 0;

		if (i == size)
			break;

		ch = buffer[i++];
		buf[offset++] = ch;

		switch (state)
		{
		case STATE_START:
		case STATE_CRLF:
			expected = '\r';
			break;
		case STATE_CR:
		case STATE_CRLFCR:
			expected = '\n';
			break;
		default:
			state = STATE_START;
			continue;
		}

		if (ch == expected)
			state++;
		else
			state = STATE_START;
	}
	*tag = i;

	// Valid End State
	if (state == STATE_CRLFCRLF)
	{
		Request *request = (Request *)malloc(sizeof(Request));
		request->header_count = 0;
		// TODO You will need to handle resizing this in parser.y
		request->headers = (Request_header *)malloc(sizeof(Request_header) * 1);
		set_parsing_options(buf, i, request);

		if (yyparse() == SUCCESS)
		{
			return request;
		}
	}
	// char bad[50] = "HTTP/1.1 400 Bad request\r\n\r\n";
	// strcpy(request->http_method,bad);
	// return request;
	// TODO Handle Malformed Requests
	printf("Parsing Failed\n");
	return NULL;
}
