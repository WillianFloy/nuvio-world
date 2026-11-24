// Regression: all catalog tiles, legacy saves, editor persistence and focus viewport.
#include "../src/central.c"
#include <assert.h>

int main(void) {
  CentralLista l = {0}, loaded;
  char buf[4096];
  int i;
  const char *old = "qualidade\ndolbyVision\ndolbyAtmos\nescolherFonteManual\n"
                    "trailerAuto\ncorDestaque\nesmaecerLocal\nvidroLocal\n";
  centrallista_ler(&loaded, old);
  assert(loaded.n == 8);
  centrallista_escrever(&loaded, buf, sizeof buf);
  assert(!strcmp(buf, old));
  for (i = 0; i < central_catalogo_n(); i++) {
    if (centrallista_alternar(&l, i) != 1) {
      fprintf(stderr, "FAIL: cannot add catalog tile %d (count=%d)\n", i + 1, l.n);
      return 1;
    }
  }
  centrallista_escrever(&l, buf, sizeof buf);
  centrallista_ler(&loaded, buf);
  assert(loaded.n == l.n && !memcmp(loaded.item, l.item, l.n * sizeof(int)));
  assert(centrallista_alternar(&l, -1) == -1);
  assert(centrallista_alternar(&l, central_catalogo_n()) == -1);
  ajustes_dir(getenv("NUVIO_DADOS"));
  dados_iniciar(getenv("NUVIO_DADOS"));
  central_teste_lista("-\n");
  central_teste_foco(-1, 1);
  {
    int offered[CENTRAL_MAX], count = ofertas(offered);
    SDL_Event e = {0};
    e.type = SDL_KEYDOWN;
    for (i = 0; i < count; i++) {
      focoEd = i;
      e.key.keysym.sym = SDLK_RETURN;
      central_evento(&e);
    }
    assert(count > 8 && lista.n == count);
    {
      char name[40], *saved;
      nomeArquivo(name, sizeof name);
      saved = dados_ler(name);
      assert(saved);
      centrallista_ler(&loaded, saved);
      free(saved);
      assert(loaded.n == lista.n && !memcmp(loaded.item, lista.item, lista.n * sizeof(int)));
    }
  }
  // C2 (World 24/11): a coluna de linhas. A rolagem tem de manter a linha em
  // foco inteira na janela, descendo e subindo, com Editar e as estatisticas.
  editando = 0;
  central_teste_estatisticas(1);
  central_teste_tocando("Playing", "1080p");
  assert(altura(1) <= CC_TETO);
  {
    float passo = CC_ROW_H + CC_ROW_GAP;
    float h = altura(1) - CC_INFO_H - 6 - CC_TOC_H - 10 - CC_PAD - 6;
    SDL_Event e = {0};
    e.type = SDL_KEYDOWN;
    e.key.keysym.sym = SDLK_DOWN;
    foco = 0;
    quadroDt = 0.016f;
    rolaBotoes = 0;
    for (i = 0; i <= botoes(NULL, NULL); i++) {
      int frame;
      central_evento(&e);
      for (frame = 0; frame < 60; frame++) {
        float top = foco * passo;
        rolaBotoes = rolagemBotoes(h);
        assert(top - rolaBotoes >= -0.01f);
        assert(top + CC_ROW_H - rolaBotoes <= h + 0.01f);
      }
    }
    assert(foco == botoes(NULL, NULL));   // a ultima linha e "Editar atalhos"
    // DIREITA numa linha de valor muda o valor e nao o foco; em Editar, nada.
    e.key.keysym.sym = SDLK_RIGHT;
    central_evento(&e);
    assert(foco == botoes(NULL, NULL));
    e.key.keysym.sym = SDLK_UP;
    for (i = 0; i <= botoes(NULL, NULL); i++) {
      float top;
      central_evento(&e);
      top = foco * passo;
      rolaBotoes = rolagemBotoes(h);
      assert(top - rolaBotoes >= -0.01f && top + CC_ROW_H - rolaBotoes <= h + 0.01f);
    }
    assert(foco == 0);
    // Janela curta: a linha em foco aparece e a centralizacao continua suave.
    foco = 9;
    rolaBotoes = 100;
    h = 400;
    {
      float alvo = 9 * passo - (h - CC_ROW_H) * 0.5f;
      float first = rolagemBotoes(h);
      assert(first > 100 && first < alvo);
      assert(9 * passo - first >= -0.01f && 9 * passo + CC_ROW_H - first <= h + 0.01f);
      rolaBotoes = first;
      assert(rolagemBotoes(h) > first);
      anim_politica_reduzida = 1;
      assert(fabsf(rolagemBotoes(h) - alvo) < 0.01f);
    }
  }
  // ESQUERDA/DIREITA mudam o valor do atalho em foco, para a frente e para tras.
  {
    int op[CENTRAL_MAX + 1], n = botoes(op, NULL), k, achou = -1;
    SDL_Event e = {0};
    anim_politica_reduzida = 0;
    for (k = 0; k < n; k++) if (op[k] != CC_OP_ESTAT && ajustes_rapido_ligado(op[k]) < 0 && op[k] != AJ_RAPIDO_COR) { achou = k; break; }
    assert(achou >= 0);
    {
      char antes[96];
      snprintf(antes, sizeof antes, "%s", ajustes_rapido_valor(op[achou]));
      foco = achou;
      e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RIGHT;
      central_evento(&e);
      assert(foco == achou && strcmp(antes, ajustes_rapido_valor(op[achou])));
      e.key.keysym.sym = SDLK_LEFT;
      central_evento(&e);
      assert(!strcmp(antes, ajustes_rapido_valor(op[achou])));
    }
  }
  puts("PASS: full catalog, legacy save, editor save/load, column viewport, smooth/reduced scroll, left/right value");
  return 0;
}
