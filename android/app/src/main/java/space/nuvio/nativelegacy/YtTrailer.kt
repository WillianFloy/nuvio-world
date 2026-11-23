package space.nuvio.nativelegacy

import android.app.Activity
import android.graphics.Color
import android.util.Log
import android.view.View
import android.view.ViewGroup
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebSettings
import android.webkit.WebView
import android.widget.FrameLayout

// TRAILER DO YOUTUBE NO BOTAO "TRAILER" (World, 23/11/2026). Pedido do dono: trailer dublado/legendado do TMDB, que e
// sempre do YouTube, tocando DENTRO do app, sem servidor. Mesmo principio da biblioteca android-youtube-player (Apache
// 2.0): o IFrame Player oficial numa WebView com origem https://www.youtube.com. Feito aqui, sem a dependencia, para
// controlar o peso numa TV de ~1 GB (TCL: 80-250 MB livres):
//   - so existe com o trailer aberto: criado no abrir, DESTRUIDO no fechar (nada fica em memoria em segundo plano);
//   - nunca no destaque nem no fundo da pagina: so o botao Trailer, em tela cheia (trailerfonte.c);
//   - sem foco nem toque: o controle continua no SDL; a pausa e a interface sao as do app (trailer.c / OSD);
//   - os controles do YouTube escondidos (controls=0), sem videos relacionados, sem anotacoes;
//   - sem WebView na TV, ou o YouTube recusando o embed (dono do video proibiu): devolve -3 e o C recua para a
//     Apple/o IMDb na mesma tela cheia (detail.c). Nada de abrir outro app por cima.
// Estado para o C (NvPlayer.ytEstado): -1 carregando, 1 tocando, 2 pausado, 3 carregando trecho, 0 terminou, -3 falhou.
object YtTrailer {
    private const val TAG = "nuvio"
    @Volatile var estado = -1; private set
    @Volatile var posMs = 0; private set
    @Volatile var durMs = 0; private set
    private var web: WebView? = null
    private var idAtual = ""
    @Volatile private var geracao = 0   // chamada atrasada de uma pagina ja fechada nao mexe no estado do trailer novo

    fun carregando() { estado = -1; posMs = 0; durMs = 0 }

    // `legenda`: idioma ("pt") da legenda do YouTube a ligar (a do video ou a traducao automatica dele); "" = sem
    // (o trailer ja esta no idioma escolhido, ou e "Legendado" com a legenda na imagem).
    fun abrir(act: Activity, camada: FrameLayout, id: String, mudo: Boolean, legenda: String) {
        fechar()
        estado = -1; posMs = 0; durMs = 0; idAtual = id
        // O id entra no HTML/JS da pagina: so o formato de id do YouTube passa.
        if (!Regex("^[A-Za-z0-9_-]{6,20}$").matches(id)) { Log.w(TAG, "[yt] id invalido"); estado = -3; return }
        val w = try { WebView(act) } catch (e: Throwable) {
            // Sem o componente WebView (ou desativado na TV): o C recua para a Apple/o IMDb.
            Log.w(TAG, "[yt] sem WebView: $e")
            estado = -3; return
        }
        w.setBackgroundColor(Color.BLACK)
        w.isFocusable = false
        w.isFocusableInTouchMode = false
        w.isClickable = false
        w.overScrollMode = View.OVER_SCROLL_NEVER
        w.isVerticalScrollBarEnabled = false
        w.isHorizontalScrollBarEnabled = false
        with(w.settings) {
            javaScriptEnabled = true
            mediaPlaybackRequiresUserGesture = false   // autoplay
            domStorageEnabled = true                   // o IFrame API guarda preferencias aqui
            cacheMode = WebSettings.LOAD_DEFAULT
            setSupportZoom(false)
            builtInZoomControls = false
            displayZoomControls = false
            loadWithOverviewMode = true
            useWideViewPort = true
        }
        w.webChromeClient = WebChromeClient()
        w.addJavascriptInterface(Ponte(++geracao), "NvYt")
        w.setLayerType(View.LAYER_TYPE_HARDWARE, null)
        // Atras da SDLSurface (a camada de video e o indice 0 do layout): o C desenha o furo e a OSD por cima.
        camada.addView(w, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        w.loadDataWithBaseURL("https://www.youtube.com", html(id, mudo, legenda), "text/html", "utf-8", null)
        web = w
        Log.i(TAG, "[yt] abrir $id mudo=$mudo legenda=$legenda")
    }

    fun comando(c: Int) {
        val w = web ?: return
        when (c) {
            1 -> w.evaluateJavascript("window.p&&p.pauseVideo&&p.pauseVideo()", null)
            2 -> w.evaluateJavascript("window.p&&p.playVideo&&p.playVideo()", null)
        }
    }

    // `fim`: o estado que fica para o C. -1 no fechar normal; 0 (terminou) quando a casca fecha sozinha (segundo
    // plano), para o C fechar sem tratar como falha.
    fun fechar(fim: Int = -1) {
        val w = web ?: return
        web = null
        geracao++
        try {
            w.evaluateJavascript("window.p&&p.stopVideo&&p.stopVideo()", null)
            w.loadUrl("about:blank")
            (w.parent as? ViewGroup)?.removeView(w)
            w.removeJavascriptInterface("NvYt")
            w.destroy()   // solta o decodificador e a memoria da pagina na hora
        } catch (e: Throwable) { Log.w(TAG, "[yt] fechar: $e") }
        estado = fim; posMs = 0; durMs = 0
        Log.i(TAG, "[yt] fechado $idAtual")
    }

    private class Ponte(val g: Int) {
        @JavascriptInterface fun estado(s: Int) {
            if (g != YtTrailer.geracao) return
            // IFrame API: -1 nao iniciou, 0 terminou, 1 tocando, 2 pausado, 3 carregando, 5 na fila.
            YtTrailer.estado = when (s) { 0 -> 0; 1 -> 1; 2 -> 2; 3 -> 3; else -> -1 }
        }
        @JavascriptInterface fun tempo(pos: Double, dur: Double) {
            if (g != YtTrailer.geracao) return
            YtTrailer.posMs = (pos * 1000).toInt(); YtTrailer.durMs = (dur * 1000).toInt()
        }
        @JavascriptInterface fun erro(codigo: Int) {
            if (g != YtTrailer.geracao) return
            // 2 id invalido, 5 HTML5, 100 removido, 101/150 o dono proibiu embutir, 152/153 configuracao do player.
            Log.w(TAG, "[yt] erro $codigo em ${YtTrailer.idAtual}")
            YtTrailer.estado = -3
        }
    }

    private fun html(id: String, mudo: Boolean, legenda: String): String {
        val lang = if (Regex("^[a-z]{2}$").matches(legenda)) legenda else ""
        val vars = buildString {
            append("autoplay:1,controls:0,rel:0,iv_load_policy:3,playsinline:1,fs:0,disablekb:1,modestbranding:1")
            append(",origin:'https://www.youtube.com'")
            append(",mute:").append(if (mudo) 1 else 0)
            if (lang.isNotEmpty()) append(",cc_load_policy:1,cc_lang_pref:'").append(lang).append("',hl:'").append(lang).append("'")
        }
        // Um so intervalo de 500 ms para o tempo (a barra da OSD); nada mais roda na pagina.
        return """<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<style>html,body{margin:0;padding:0;background:#000;overflow:hidden;width:100%;height:100%}#p{width:100%;height:100%}</style>
</head><body><div id="p"></div><script>
var p=null;
function onYouTubeIframeAPIReady(){
 p=new YT.Player('p',{width:'100%',height:'100%',videoId:'$id',playerVars:{$vars},
  events:{
   onReady:function(e){e.target.playVideo();},
   onStateChange:function(e){NvYt.estado(e.data);},
   onError:function(e){NvYt.erro(e.data);}
  }});
 setInterval(function(){try{if(p&&p.getCurrentTime)NvYt.tempo(p.getCurrentTime()||0,p.getDuration()||0);}catch(x){}},500);
}
</script><script src="https://www.youtube.com/iframe_api"></script></body></html>"""
    }
}
