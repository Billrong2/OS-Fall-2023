#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
int total_ticket;


void store_tickets(struct proc *p)
{
    total_ticket -= p->tickets;
}
void get_tickets(struct proc *p)
{
    total_ticket += p->tickets;
}
void set_total_tickets(){
    total_ticket = get_total_tickets();
}

int set_tickets(struct proc *p, int a){

        store_tickets(p);
        p->tickets = a;
        get_tickets(p);
    return total_ticket;
}
/* 
    File added by rxk and ysq
*/