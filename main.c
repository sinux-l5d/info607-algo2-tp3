#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "points.h"
#include "particules.h"
#include "forces.h"

//-----------------------------------------------------------------------------
// Déclaration des types
//-----------------------------------------------------------------------------
/**
   Le contexte contient les informations utiles de l'interface pour
   les algorithmes de géométrie algorithmique.
*/
typedef struct SContexte
{
  int width;
  int height;
  GtkWidget *drawing_area;
  TabParticules TabP;
  Force forces[NB_FORCES];
  GtkWidget *label_nb;
  GtkWidget *label_distance;
} Contexte;

// Pas de temps en s
#define DT 0.01
// Pas de temps en s pour le réaffichage
#define DT_AFF 0.02

//-----------------------------------------------------------------------------
// Déclaration des fonctions
//-----------------------------------------------------------------------------
/**
   Crée l'interface graphique en fonction du contexte \a pCtxt.
*/
GtkWidget *creerIHM(Contexte *pCtxt);

/**
   c'est la réaction principale qui va redessiner tout.
*/
gboolean on_draw(GtkWidget *widget, GdkEventExpose *event, gpointer data);

/**
   Fait la conversion coordonnées réelles de \a p vers coordonnées de la zone de dessin.
   @param pCtxt le contexte de l'IHM
   @param p le point en entrée
   @return ses coordonnées dans la zone de dessin.
*/
Point point2DrawingAreaPoint(Contexte *pCtxt, Point p);

/**
   Fait la conversion longueur réelle \a l vers longueur en pixel dans la zone de dessin.
   @param pCtxt le contexte de l'IHM
   @param l la longueur réelle
   @return la longueur correspondante en pixels.
*/
double length2DrawingAreaLength(Contexte *pCtxt, double l);

/**
   Fait la conversion coordonnées pixel de \a p vers coordonnées réelles.
   @param pCtxt le contexte de l'IHM
   @param p les coordonnées pixel en entrée
   @return ses coordonnées réelles.
*/
Point drawingAreaPoint2Point(Contexte *pCtxt, Point p);

/**
   Affiche un point \a p dans une zone de dessin cairo \a cr comme un disque.
   La masse influe sur la taille d'affichage de la particule.

   @param cr le contexte CAIRO pour dessiner dans une zone de dessin.
   @param p un point dans la zone de dessin.
 */
void drawParticule(Contexte *pCtxt, cairo_t *cr, Particule p);

/**
   Fonction de base qui affiche un disque de centre (x,y) et de rayon r via cairo.
*/
void drawPoint(cairo_t *cr, double x, double y, double r);

/**
   Fonction appelée régulièrement (tous les DT secondes) et qui s'occupe de (presque tout):
   - générer de nouvelles particules: \ref fontaine
   - calculer les forces sur chaque particule: \ref calculDynamique
   - déplacer les particules et gérer les collisions: \ref deplaceTout

   @param data correspond en fait au pointeur vers le Contexte.
*/
gint tic(gpointer data);

/**
   Fonction appelée régulièrement (tous les DT_AFF secondes) et qui
   s'occupe de demander le réaffichage dela zone de dessin.

   @param data correspond en fait au pointeur vers le Contexte.
*/
gint ticAffichage(gpointer data);

/**
   Fonction appelée régulièrement (tous les secondes) et qui
   affiche le nombre d'appels à la fonction \c distance par seconde.

   @param data correspond en fait au pointeur vers le Contexte.
*/
gint ticDistance(gpointer data);

/**
   Calcul la dynamique de tous les points en appliquant les forces et
   met à jour la vitesse.
*/
void calculDynamique(Contexte *pCtxt);

/**
   Déplace toutes les particules en fonction de leur vitesse.
*/
void deplaceTout(Contexte *pCtxt);

/**
   Déplace une particule en fonction de sa vitesse. Devra s'occuper
   des collisions plus tard.
*/
void deplaceParticule(Contexte *pCtxt, Particule *p);

/**
  Fontaine pour créer une particule à la position (\a x, \a y), avec
  la vitesse (\a vx, \a vy) et la masse \a m.

  @param p probabilité (entre 0 et 1) qu'une particule soit effectivement créée.
  @param x la coordonnée x de la position où la particule est créée.
  @param y la coordonnée y de la position où la particule est créée.
  @param vx la composante x de la vitesse de la particule.
  @param vy la composante y de la vitesse de la particule.
  @param m la masse de la particule créée.
 */
void fontaine(Contexte *pCtxt, double p,
              double x, double y, double vx, double vy, double m);

//-----------------------------------------------------------------------------
// Programme principal
//-----------------------------------------------------------------------------
int main(int argc,
         char *argv[])
{
  Contexte context;
  TabParticules_init(&context.TabP);

  /* Passe les arguments à GTK, pour qu'il extrait ceux qui le concernent. */
  gtk_init(&argc, &argv);

  /* Crée une fenêtre. */
  creerIHM(&context);

  /* Rentre dans la boucle d'événements. */
  gtk_main();
  return 0;
}

gboolean
on_draw(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  // c'est la réaction principale qui va redessiner tout.
  Contexte *pCtxt = (Contexte *)data;
  TabParticules *ptrP = &(pCtxt->TabP);
  // c'est la structure qui permet d'afficher dans une zone de dessin
  // via Cairo
  GdkWindow *window = gtk_widget_get_window(widget);
  cairo_region_t *cairoRegion = cairo_region_create();
  GdkDrawingContext *drawingContext = gdk_window_begin_draw_frame(window, cairoRegion);
  cairo_t *cr = gdk_drawing_context_get_cairo_context(drawingContext);
  cairo_set_source_rgb(cr, 1, 1, 1); // choisit le blanc.
  cairo_paint(cr);                   // remplit tout dans la couleur choisie.

  // Affiche tous les points en bleu.
  cairo_set_source_rgb(cr, 0, 0, 1);
  for (int i = 0; i < TabParticules_nb(ptrP); ++i)
  {
    drawParticule(pCtxt, cr, TabParticules_get(ptrP, i));
  }

  // On a fini, on peut détruire la structure.
  gdk_window_end_draw_frame(window, drawingContext);
  // cleanup
  cairo_region_destroy(cairoRegion);
  return TRUE;
}

Point point2DrawingAreaPoint(Contexte *pCtxt, Point p)
{
  Point q;
  q.x[0] = (p.x[0] + 1.0) / 2.0 * pCtxt->width;
  q.x[1] = (1.0 - p.x[1]) / 2.0 * pCtxt->height;
  return q;
}

double length2DrawingAreaLength(Contexte *pCtxt, double l)
{
  return pCtxt->width * l / 2.0;
}

Point drawingAreaPoint2Point(Contexte *pCtxt, Point p)
{
  Point q;
  q.x[0] = 2.0 * ((double)p.x[0] / (double)pCtxt->width) - 1.0;
  q.x[1] = -2.0 * ((double)p.x[1] / (double)pCtxt->height) + 1.0;
  return q;
}

void drawParticule(Contexte *pCtxt, cairo_t *cr, Particule p)
{
  Point pp;
  pp.x[0] = p.x[0];
  pp.x[1] = p.x[1];
  // On convertit les coordonnées réelles des particules (dans [-1:1]x[-1:1]) en coordonnées
  // de la zone de dessin (dans [0:499]x[0:499]).
  Point q = point2DrawingAreaPoint(pCtxt, pp);
  drawPoint(cr, q.x[0], q.x[1], 1.5 * sqrt(p.m));
}

void drawPoint(cairo_t *cr, double x, double y, double r)
{
  cairo_arc(cr, x, y, r, 0.0, 2.0 * 3.14159626);
  cairo_fill(cr);
}

void drawLine(cairo_t *cr, Point p, Point q)
{
  cairo_move_to(cr, p.x[0], p.x[1]);
  cairo_line_to(cr, q.x[0], q.x[1]);
  cairo_stroke(cr);
}

/// Charge l'image donnée et crée l'interface.
GtkWidget *creerIHM(Contexte *pCtxt)
{
  GtkWidget *window;
  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *hbox1;
  GtkWidget *button_quit;

  /* Crée une fenêtre. */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // Crée un conteneur horizontal box.
  hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  // Crée deux conteneurs vertical box.
  vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  // Crée une zone de dessin
  pCtxt->drawing_area = gtk_drawing_area_new();
  pCtxt->width = 500;
  pCtxt->height = 500;
  gtk_widget_set_size_request(pCtxt->drawing_area, pCtxt->width, pCtxt->height);
  // Crée le pixbuf source et le pixbuf destination
  gtk_container_add(GTK_CONTAINER(hbox1), pCtxt->drawing_area);
  // ... votre zone de dessin s'appelle ici "drawing_area"
  g_signal_connect(G_OBJECT(pCtxt->drawing_area), "draw",
                   G_CALLBACK(on_draw), pCtxt);
  // Rajoute le 2eme vbox dans le conteneur hbox (pour mettre les boutons sélecteur de points
  gtk_container_add(GTK_CONTAINER(hbox1), vbox2);
  // Crée les labels pour afficher le nombre de points et le nombre
  // d'appel à la fonction distance.
  pCtxt->label_nb = gtk_label_new("0 points");
  gtk_container_add(GTK_CONTAINER(vbox2), pCtxt->label_nb);
  pCtxt->label_distance = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(vbox2), pCtxt->label_distance);

  // Crée le bouton quitter.
  button_quit = gtk_button_new_with_label("Quitter");
  // Connecte la réaction gtk_main_quit à l'événement "clic" sur ce bouton.
  g_signal_connect(button_quit, "clicked",
                   G_CALLBACK(gtk_main_quit),
                   NULL);
  // Rajoute tout dans le conteneur vbox.
  gtk_container_add(GTK_CONTAINER(vbox1), hbox1);
  gtk_container_add(GTK_CONTAINER(vbox1), button_quit);
  // Rajoute la vbox  dans le conteneur window.
  gtk_container_add(GTK_CONTAINER(window), vbox1);

  // Rend tout visible
  gtk_widget_show_all(window);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Crée les forces
  Force g = gravite(0.0, -0.2);
  pCtxt->forces[0] = g;

  // enclenche le timer pour se déclencher dans 5ms.
  g_timeout_add(1000 * DT, tic, (gpointer)pCtxt);
  // enclenche le timer pour se déclencher dans 20ms.
  g_timeout_add(1000 * DT_AFF, ticAffichage, (gpointer)pCtxt);
  // enclenche le timer pour se déclencher dans 1000ms.
  g_timeout_add(1000, ticDistance, (gpointer)pCtxt);

  return window;
}

void fontaine(Contexte *pCtxt,
              double p, double x, double y, double vx, double vy, double m)
{
  TabParticules *P = &pCtxt->TabP;
  if ((rand() / (double)RAND_MAX) < p)
  {
    Particule q;
    initParticule(&q, x, y, vx, vy, m);
    TabParticules_ajoute(P, q);
  }
}

gint tic(gpointer data)
{
  Contexte *pCtxt = (Contexte *)data;
  fontaine(pCtxt, 0.25, -0.5, 0.5, 0.3, 0.3, 2.5);
  calculDynamique(pCtxt);
  deplaceTout(pCtxt);
  g_timeout_add(1000 * DT, tic, (gpointer)pCtxt); // réenclenche le timer.
  return 0;
}

gint ticAffichage(gpointer data)
{
  Contexte *pCtxt = (Contexte *)data;
  char buffer[128];
  sprintf(buffer, "%d points", TabParticules_nb(&pCtxt->TabP));
  gtk_label_set_text(GTK_LABEL(pCtxt->label_nb), buffer);
  gtk_widget_queue_draw(pCtxt->drawing_area);
  g_timeout_add(1000 * DT_AFF, ticAffichage, (gpointer)pCtxt); // réenclenche le timer.
  return 0;
}

gint ticDistance(gpointer data)
{
  Contexte *pCtxt = (Contexte *)data;
  char buffer[128];
  sprintf(buffer, "%7d nb appels à distance()", getCompteurDistance()),
      gtk_label_set_text(GTK_LABEL(pCtxt->label_distance), buffer);
  resetCompteurDistance();
  g_timeout_add(1000, ticDistance, (gpointer)pCtxt); // réenclenche le timer.
  return 0;
}

void calculDynamique(Contexte *pCtxt)
{
  TabParticules *P = &pCtxt->TabP;
  int n = TabParticules_nb(P);
  Force *F = pCtxt->forces;
  // On met à zéro les forces de chaque point.
  for (int i = 0; i < n; ++i)
  {
    Particule *p = TabParticules_ref(P, i);
    p->f[0] = 0.0;
    p->f[1] = 0.0;
  }
  // On applique les forces à tous les points
  for (int i = 0; i < n; ++i)
  {
    Particule *p = TabParticules_ref(P, i);
    for (int j = 0; j < NB_FORCES; ++j)
      appliqueForce(p, &F[j]);
  }
  // On applique la loi de Newton: masse*acceleration = somme des forces
  // ie m dv/dt = sum f
  // ie v[t+dt] = v[t] + (dt/m) * sum f
  for (int i = 0; i < n; ++i)
  {
    Particule *p = TabParticules_ref(P, i);
    p->v[0] += (DT / p->m) * p->f[0];
    p->v[1] += (DT / p->m) * p->f[1];
  }
}

void deplaceParticule(Contexte *pCtxt, Particule *p)
{
  /* Déplace p en supposant qu'il n'y a pas de collision. */
  p->x[0] += DT * p->v[0];
  p->x[1] += DT * p->v[1];
}

void deplaceTout(Contexte *pCtxt)
{
  TabParticules *P = &pCtxt->TabP;
  int n = TabParticules_nb(P);
  // Applique le vecteur vitesse sur toutes les particules.
  for (int i = 0; i < n; ++i)
  {
    Particule *p = TabParticules_ref(P, i);
    deplaceParticule(pCtxt, p);
  }
  // Détruit les particules trop loin de la zone
  for (int i = 0; i < TabParticules_nb(P);)
  {
    Particule *p = TabParticules_ref(P, i);
    if ((p->x[0] < -1.5) || (p->x[0] > 1.5) || (p->x[1] < -1.5) || (p->x[1] > 1.5))
      TabParticules_supprime(P, i);
    else
      ++i;
  }
}
