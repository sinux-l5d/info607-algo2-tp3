#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "obstacles.h"

void Obstacle_init(Obstacle *p, double r, double x, double y, double att)
{
    p->type = DISQUE;
    p->x[0] = x;
    p->x[1] = y;
    p->r = r;
    p->att = att;
    p->cr = 0;
    p->cb = 0;
    p->cg = 0;
}

void TabObstacles_init(TabObstacles *tab)
{
    tab->taille = 10;
    tab->nb = 0;
    tab->obstacles = (Obstacle *)malloc(tab->taille * sizeof(Obstacle));
}

void TabObstacles_ajoute(TabObstacles *tab, Obstacle p)
{
    if (tab->nb == tab->taille)
    {
        TabObstacles_agrandir(tab);
    }
    tab->obstacles[tab->nb] = p;
    tab->nb++;
}

void TabObstacles_set(TabObstacles *tab, int i, Obstacle p)
{
    assert(i < tab->nb);
    tab->obstacles[i] = p;
}

Obstacle TabObstacles_get(TabObstacles *tab, int i)
{
    assert(i < tab->nb);
    return tab->obstacles[i];
}
int TabObstacles_nb(TabObstacles *tab)
{
    return tab->nb;
}

void TabObstacles_termine(TabObstacles *tab)
{
    free(tab->obstacles);
    tab->nb = 0;
    tab->taille = 0;
}

Obstacle *TabObstacles_ref(TabObstacles *tab, int i)
{
    assert(i < tab->nb);
    return tab->obstacles + i;
}

void TabObstacles_agrandir(TabObstacles *tab)
{
    tab->taille *= 2;
    tab->obstacles = (Obstacle *)realloc(tab->obstacles, tab->taille * sizeof(Obstacle));
}

void TabObstacles_affiche(TabObstacles *tab)
{
    int i;
    printf("TAB OBSTACLES =========================\n");
    for (i = 0; i < tab->nb; i++)
    {
        printf("Obstacle %d :\n", i);
        printf("\ttype : %d\n", tab->obstacles[i].type);
        printf("\tr : %f\n", tab->obstacles[i].r);
        printf("\tx : %f\n", tab->obstacles[i].x[0]);
        printf("\ty : %f\n", tab->obstacles[i].x[1]);
        printf("\tatt : %f\n", tab->obstacles[i].att);
        printf("\tcr : %f\n", tab->obstacles[i].cr);
        printf("\tcb : %f\n", tab->obstacles[i].cb);
        printf("\tcg : %f\n", tab->obstacles[i].cg);
    }
    printf("========================================\n");
}