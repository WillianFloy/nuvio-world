#include "chorigem.h"
#ifndef CHORIGEM_SO_REGRA   // tests/chorigem.sh compila so a regra pura
#include "ajustes.h"
#include "app.h"
#include "home.h"
#include "ilha.h"
#include "menu.h"
#include "layout.h"
#include "escala.h"

#endif

ChOrigemTipo chorigem_decidir(ChPainel p, int ajuste, int buscaDaBarra, int topoVisivel, int relogioVisivel) {
  if (ajuste == 2) return CHO_CENTRO;
  if (ajuste == 1) return relogioVisivel ? CHO_RELOGIO : CHO_CENTRO;
  if (topoVisivel && (p != CHO_P_BUSCA || buscaDaBarra)) return CHO_TOPO;
  if (relogioVisivel) return CHO_RELOGIO;
  return CHO_CENTRO;
}

GfxRect chorigem_centro(float w) {
  return (GfxRect){ w * 0.5f - 60.0f, 40.0f, 120.0f, 52.0f };
}
#ifndef CHORIGEM_SO_REGRA

// O menu do topo esta na tela (fixo, ou o nao fixo no destaque da Home).
static int forcaHome = -1;   // testes: sem o app de verdade nao ha "na Home"
void chorigem_teste_na_home(int sim) { forcaHome = sim; }
static int topoVisivel(void) {
  if (!ajustes_menu_topo() || !menu_visivel() || menu_aberto()) return 0;
  return (forcaHome >= 0 ? forcaHome : app_na_home()) && home_topo_fracao() > 0.5f;
}

ChOrigemTipo chorigem(ChPainel p, GfxRect *r) {
  float ix, iy, iw, ih;
  int rel = ilha_rect(&ix, &iy, &iw, &ih) && iw > 4.0f && ih > 4.0f;
  int topo = topoVisivel();
  ChOrigemTipo t = chorigem_decidir(p, ajustes_ch_origem(), ajustes_busca_da_barra(), topo, rel);
  GfxRect o = chorigem_centro(NV_LAYOUT_REAL_W);
  // A pilula e medida na camada ampliada dela (ESCALA_INI): para px reais.
  { float u = gfx_escala_ui(); ix *= u; iy *= u; iw *= u; ih *= u; }
  // A CENTRAL E O RELOGIO SAO A MESMA PECA: com o menu do topo na tela a
  // pilula do relogio mora na ponta dele, e a Central (que sempre foi a ilha
  // esticada) nasce dela; sem relogio, da barra.
  if (t == CHO_TOPO && p == CHO_P_CENTRAL && rel) t = CHO_RELOGIO;
  if (t == CHO_TOPO) {
    MenuTopoGeo g;
    int k;
    menu_topo_geo(&g);
    o = (GfxRect){ g.trilha[0], g.trilha[1], g.trilha[2], g.trilha[3] };
    if (p == CHO_P_BUSCA)
      for (k = 0; k < g.n; k++) if (g.busca[k]) { o = (GfxRect){ g.x[k], g.y, g.w[k], g.h }; break; }
    if (o.w < 4.0f || o.h < 4.0f) { t = rel ? CHO_RELOGIO : CHO_CENTRO; o = chorigem_centro(NV_LAYOUT_REAL_W); }
  }
  if (t == CHO_RELOGIO) o = (GfxRect){ ix, iy, iw, ih };
  if (r) *r = o;
  return t;
}
#endif
