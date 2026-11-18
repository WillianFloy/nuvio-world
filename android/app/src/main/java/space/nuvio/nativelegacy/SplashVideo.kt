package space.nuvio.nativelegacy

import android.app.Activity
import android.graphics.Color
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import android.view.SurfaceView
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.media3.common.MediaItem
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import java.io.File

// VIDEO DE ABERTURA NUVIO WORLD (res/raw/abertura_world.mp4).
//
// O video toca POR CIMA de tudo enquanto o nucleo C compila os shaders e monta a
// home por baixo — a abertura nativa (abertura.c) continua existindo e e o
// fallback. Duas camadas, ambas so durante a abertura:
//   - `capa`: uma View preta na janela. A SDLSurface e media overlay (atras da
//     janela), entao a capa a esconde; no fim ela esmaece e revela o app.
//   - `tela`: a SurfaceView do video com setZOrderOnTop, acima da janela.
//
// Linha do tempo do arquivo (medida quadro a quadro, 24 fps, 10 s):
//   0,4 s  o play aparece         2,6 s  o N esta formado (INTRO_MS)
//   2,6-7,6 s  o N parado, crescendo devagar, com um brilho
//   7,6 s  comeca o recolhimento  ~9,2 s  some (FIM_MS)
// Regras: nunca se espera o video alem do N formado. Com o app pronto (a
// abertura nativa acabou: nativeAberturaPronta) e o N formado, o video salta
// para o recolhimento — num quadro-chave (7,875 s / 8,0 s) cuja largura casa com
// a do N naquele instante, para o salto nao se ver. Se o app demora, o video para
// no N (7,6 s) e espera parado; quando ficar pronto, segue dali. Qualquer tecla
// pula para o recolhimento. Sem primeiro quadro em 2,5 s, erro do decoder ou
// 15 s no total: a capa sai na hora e fica a abertura de sempre.
class SplashVideo(
    private val act: Activity,
    private val pronto: () -> Boolean,
    private val aoTerminar: () -> Unit,
) {
    companion object {
        private const val TAG = "Nuvio"
        private const val INTRO_MS = 2600L
        private const val ESPERA_MS = 7600L
        private const val FIM_MS = 9200L
        private const val SEM_QUADRO_MS = 2500L
        private const val TETO_MS = 15000L
        private const val CAPA_SAI_MS = 280L

        // Ajustes > Aparencia > "Video de abertura" (ajustes.c, aberturaVideoLocal):
        // V_LIGA, 0 = Ligado (padrao), 1 = Desligado. Lido do ajustes.txt antes do C.
        fun ligado(dados: File): Boolean = try {
            var v = 0
            File(dados, "ajustes.txt").useLines { linhas ->
                for (l in linhas) {
                    val f = l.trim().split(Regex("\\s+"), limit = 3)
                    if (f.size >= 2 && f[0] == "aberturaVideoLocal") v = f[1].toIntOrNull() ?: 0
                }
            }
            v != 1
        } catch (_: Exception) { true }
    }

    private val main = Handler(Looper.getMainLooper())
    private var player: ExoPlayer? = null
    private var tela: SurfaceView? = null
    private var capa: View? = null
    private var inicio = 0L
    private var primeiroQuadro = false
    private var saindo = false        // ja pediu o recolhimento
    private var acabou = false
    private var parado = false        // segurando no N (7,6 s)

    val ativo: Boolean get() = !acabou

    fun iniciar(raiz: ViewGroup) {
        inicio = SystemClock.uptimeMillis()
        val c = View(act).apply { setBackgroundColor(Color.BLACK) }
        val s = SurfaceView(act).apply { setZOrderOnTop(true) }
        raiz.addView(c, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        raiz.addView(s, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        capa = c; tela = s
        try {
            val p = ExoPlayer.Builder(act).build()
            player = p
            p.volume = 0f
            p.setVideoSurfaceView(s)
            p.addListener(object : Player.Listener {
                override fun onRenderedFirstFrame() {
                    primeiroQuadro = true
                    Log.i(TAG, "splash: primeiro quadro em ${SystemClock.uptimeMillis() - inicio} ms")
                }
                override fun onPlayerError(error: PlaybackException) {
                    Log.w(TAG, "splash: erro ${error.errorCodeName}; abertura nativa")
                    terminar(imediato = true)
                }
                override fun onPlaybackStateChanged(state: Int) {
                    if (state == Player.STATE_ENDED) terminar(imediato = false)
                }
            })
            p.setMediaItem(MediaItem.fromUri(Uri.parse("android.resource://${act.packageName}/${R.raw.abertura_world}")))
            p.prepare()
            p.playWhenReady = true
        } catch (e: Exception) {
            Log.w(TAG, "splash: nao iniciou ($e); abertura nativa")
            terminar(imediato = true)
            return
        }
        main.post(tique)
    }

    // Uma tecla: vai para o recolhimento (ou sai na hora, se nem comecou).
    fun pular() {
        if (acabou) return
        if (!primeiroQuadro) { terminar(imediato = true); return }
        recolher()
    }

    private fun recolher() {
        val p = player ?: return
        if (saindo) { if (parado) { parado = false; p.play() }; return }
        saindo = true
        val pos = p.currentPosition
        if (pos < ESPERA_MS) {
            // Quadro-chave cuja largura do N casa com a do instante (o N cresce de
            // 56 a 62 de 2,6 a 7,6 s; o recolhimento passa por 60 em 7,875 e 58 em 8,0).
            p.seekTo(if (pos < 5000L) 8000L else 7875L)
        }
        if (parado) { parado = false }
        p.play()
        Log.i(TAG, "splash: recolhendo em ${SystemClock.uptimeMillis() - inicio} ms (video ${pos} ms)")
    }

    private val tique = object : Runnable {
        override fun run() {
            if (acabou) return
            val agora = SystemClock.uptimeMillis() - inicio
            val p = player
            if (p == null || agora > TETO_MS || (!primeiroQuadro && agora > SEM_QUADRO_MS)) {
                Log.w(TAG, "splash: ${if (!primeiroQuadro) "sem quadro" else "teto"} em $agora ms; abertura nativa")
                terminar(imediato = !primeiroQuadro)
                return
            }
            val pos = p.currentPosition
            if (pos >= FIM_MS) { terminar(imediato = false); return }
            if (!saindo) {
                if (pos >= INTRO_MS && pronto()) recolher()
                else if (pos >= ESPERA_MS && !parado) { parado = true; p.pause() }
            } else if (parado && pronto()) { parado = false; p.play() }
            main.postDelayed(this, 33L)
        }
    }

    private fun terminar(imediato: Boolean) {
        if (acabou) return
        acabou = true
        main.removeCallbacks(tique)
        try { player?.release() } catch (_: Exception) {}
        player = null
        tela?.let { (it.parent as? ViewGroup)?.removeView(it) }
        tela = null
        val c = capa
        capa = null
        Log.i(TAG, "splash: fim em ${SystemClock.uptimeMillis() - inicio} ms")
        if (c == null) { aoTerminar(); return }
        if (imediato) { (c.parent as? ViewGroup)?.removeView(c); aoTerminar(); return }
        c.animate().alpha(0f).setDuration(CAPA_SAI_MS).withEndAction {
            (c.parent as? ViewGroup)?.removeView(c)
            aoTerminar()
        }.start()
    }

    fun encerrar() { terminar(imediato = true) }
}
