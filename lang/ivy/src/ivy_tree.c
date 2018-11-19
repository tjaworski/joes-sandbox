/* IVY abstract syntax trees

   Copyright (C) 1993 Joseph H. Allen

This file is part of IVY

IVY is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 1, or (at your option) any later version.

IVY is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
IVY; see the file COPYING.  If not, write to the Free Software Foundation,
675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "ivy_tree.h"
#include "free_list.h"
#include "ivy.h"

/* Trees are short-lived in Ivy: they exist between the parser and the
   code generator.  A single tree is never larger than a single top-level
   block. */

/* Construct a two operand node: note that NULL is returned if any
   of the arguments are NULL. */

Node *cons2(Loc *loc, int what, Node *left, Node *right)
{
	if (!left || !right) {
		rm(loc, right);
		rm(loc, left);
		return 0;
	} else if (what == nSEMI && left->what == nSEMI) {
		/* Build a list */
		Node *n;
		for (n = left; n->r && n->r->what == nSEMI; n = n->r);
		n->r = cons2(loc, nSEMI, n->r, right);
		return left;
	} else {
		Node *n = (Node *)al_item(loc->free_list);
		n->what = what;
		n->l = left;
		n->r = right;
		n->s = 0;
		*n->loc = *loc;
		return n;
	}
}

/* Get first node of list */

Node *first(Node *list)
{
	if (list && list->what == nSEMI)
		return list->l;
	else
		return list;
}

/* Get rest of list */

Node *rest(Node *list)
{
	if (list && list->what == nSEMI)
		return list->r;
	else
		return 0;
}

/* Construct a single operand node: note that NULL is returned if any
   of the arguments are NULL. */

Node *cons1(Loc *loc,int what, Node *right)
{
	if (!right)
		return 0;
	else {
		Node *n = (Node *)al_item(loc->free_list);
		n->what = what;
		n->l = 0;
		n->r = right;
		n->s = 0;
		*n->loc = *loc;
		return n;
	}
}

/* Construct an integer constant */

Node *consnum(Loc *loc,long long v)
{
	Node *n = (Node *)al_item(loc->free_list);
	n->what = nNUM;
	n->l = 0;
	n->r = 0;
	n->n = v;
	n->s = 0;
	*n->loc = *loc;
	return n;
}

/* Construct a floating point constant */

Node *consfp(Loc *loc,double v)
{
	Node *n = (Node *)al_item(loc->free_list);
	n->what = nFP;
	n->l = 0;
	n->fp = v;
	n->r = 0;
	n->n = 0;
	n->s = 0;
	*n->loc = *loc;
	return n;
}

/* Construct a string family node */

Node *conss(Loc *loc,int a, char *v, int l)
{
	Node *n = (Node *)al_item(loc->free_list);
	n->what = a;
	n->l = 0;
	n->r = 0;
	n->s = v;
	n->n = l;
	*n->loc = *loc;
	return n;
}

/* Construct a string constant */

Node *consstr(Loc *loc, char *v, int len)
{
	return conss(loc, nSTR, v, len);
}

/* Construct an identifier */

Node *consnam(Loc *loc, char *v)
{
	return conss(loc, nNAM, v, strlen(v));
}

/* Construct a label */

Node *conslabel(Loc *loc, char *v)
{
	return conss(loc, nLABEL, v, strlen(v));
}

/* Construct a void */

Node *consvoid(Loc *loc)
{
	Node *n = (Node *)al_item(loc->free_list);
	n->what = nVOID;
	n->l = 0;
	n->r = 0;
	n->s = 0;
	n->n = 0;
	*n->loc = *loc;
	return n;
}

Node *consthis(Loc *loc)
{
	Node *n = (Node *)al_item(loc->free_list);
	n->what = nTHIS;
	n->l = 0;
	n->r = 0;
	n->s = 0;
	n->n = 0;
	*n->loc = *loc;
	return n;
}

/* Construct empty */

Node *consempty(Loc *loc)
{
	Node *n = (Node *)al_item(loc->free_list);
	n->what = nEMPTY;
	n->l = 0;
	n->r = 0;
	n->s = 0;
	n->n = 0;
	*n->loc = *loc;
	return n;
}

/* Construct an optional node */

Node *opt(Loc *loc, Node *n)
{
	if (n)
		return n;
	else
		return consempty(loc);
}

/* Duplicate a tree */

Node *dup(Loc *loc, Node * o)
{
	Node *n;
	if (!o)
		return 0;
	n = (Node *)al_item(loc->free_list);
	n->what = o->what;
	if (o->s && o->what == nNAM) {
		n->s = o->s;
	} else if (o->s) {
		int x;
		n->s = (char *) malloc(o->n + 1);
		for (x = 0; x != o->n; ++x)
			n->s[x] = o->s[x];
		n->s[x] = 0;
	} else
		n->s = 0;
	n->n = o->n;
	n->r = dup(loc, o->r);
	n->l = dup(loc, o->l);
	*n->loc = *o->loc;
	return n;
}

/* Eliminate a tree */

void rm(Loc *loc, Node *n)
{
	if (n) {
		rm(loc, n->l);
		rm(loc, n->r);
		if (n->s && n->what != nNAM)
			free(n->s);
		fr_item(loc->free_list, n);
	}
}

/* Tree printer */

void indent(FILE *out, int x)
{
	while (x--)
		fputc(' ', out);
}

void prtree(FILE *out, Node *n, int lvl)
{
	switch (n->what) {
		case nNUM: {
			indent(out, lvl), fprintf(out, "%lld", n->n);
			break;
		} case nFP: {
			indent(out, lvl), fprintf(out, "%g", n->fp);
			break;
		} case nSTR: {
			indent(out, lvl), fprintf(out, "\"%s\"", n->s);
			break;
		} case nVOID: {
			indent(out, lvl), fprintf(out, "void");
			break;
		} default: {
			if (n->s)
				indent(out, lvl), fprintf(out, "%s", n->s);
			else {
				indent(out, lvl), fprintf(out, "(%s\n", what_tab[n->what].name);
				if (n->l)
					prtree(out, n->l, lvl + 2);
				if (n->r)
					prtree(out, n->r, lvl + 2);
				indent(out, lvl), fprintf(out, ")");
			}
		}
	}
	fprintf(out, "\n");
}
