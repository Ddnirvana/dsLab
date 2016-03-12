/*
 * add by Dd
 * origin from then message struct
 * changed it to a chain of all the message
 */
struct Dd_windows 
	int id;{
	int size;
	char *data;
	struct Dd_windows * next;
};

