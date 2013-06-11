/* Dhara - NAND flash management layer
 * Copyright (C) 2013 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dhara/journal.h"
#include "sim.h"
#include "util.h"
#include "jtutil.h"

static void suspend_resume(struct dhara_journal *j)
{
	const dhara_page_t old_root = dhara_journal_root(j);
	const dhara_page_t old_tail = dhara_journal_tail(j);
	const dhara_page_t old_head = j->head;
	dhara_error_t err;

	j->root = DHARA_PAGE_NONE;
	j->head = DHARA_PAGE_NONE;
	j->tail = DHARA_PAGE_NONE;

	if (dhara_journal_resume(j, &err) < 0)
		dabort("resume", err);

	assert(old_root == dhara_journal_root(j));
	assert(old_tail == dhara_journal_tail(j));
	assert(old_head == j->head);
}

static void dump_info(struct dhara_journal *j)
{
	printf("    log2_ppc   = %d\n", j->log2_ppc);
	printf("    size       = %d\n", dhara_journal_size(j));
	printf("    capacity   = %d\n", dhara_journal_capacity(j));
	printf("    bb_current = %d\n", j->bb_current);
	printf("    bb_last    = %d\n", j->bb_last);
}

int main(void)
{
	struct dhara_journal journal;
	const size_t page_size = 1 << sim_nand.log2_page_size;
	uint8_t page_buf[page_size];
	int i;
	int rep;

	sim_reset();
	sim_inject_bad(20);

	printf("Journal init\n");
	dhara_journal_init(&journal, &sim_nand, page_buf);
	dump_info(&journal);
	printf("\n");

	printf("Enqueue/dequeue, 100 pages x20\n");
	for (rep = 0; rep < 20; rep++) {
		for (i = 0; i < 100; i++)
			jt_enqueue(&journal, i);
		printf("    size     = %d -> ", dhara_journal_size(&journal));
		for (i = 0; i < 100; i++)
			jt_dequeue(&journal, i);
		printf("%d\n", dhara_journal_size(&journal));
	}
	printf("\n");

	printf("Journal stats:\n");
	dump_info(&journal);
	printf("\n");

	printf("Enqueue/dequeue, ~100 pages x20 (resume)\n");
	for (rep = 0; rep < 20; rep++) {
		int j;

		for (i = 0; i < 100; i++)
			jt_enqueue(&journal, i);

		while (!dhara_journal_is_checkpointed(&journal))
			jt_enqueue(&journal, i++);

		printf("    size     = %d -> ", dhara_journal_size(&journal));
		suspend_resume(&journal);

		for (j = 0; j < i; j++)
			jt_dequeue(&journal, j);
		printf("%d\n", dhara_journal_size(&journal));
	}
	printf("\n");

	printf("Journal stats:\n");
	dump_info(&journal);
	printf("\n");

	sim_dump();

	return 0;
}