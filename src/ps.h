/*
 * ps.h
 *
 *  Created on: Jun 17, 2010
 *      Author: Thomás Alimena Del Grande (tgrande@pd3.com.br)
 */

#ifndef PS_H_
#define PS_H_

struct process_t {
	int pid;
	char name[64];
	char up_time[64];
	char user[64];
	struct process_t *next;
};

struct process_t *librouter_ps_get_info(void);
void librouter_ps_free_info(struct process_t *ps);

#endif /* PS_H_ */
