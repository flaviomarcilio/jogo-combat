#include <stdio.h>
#include <math.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#define DEBUG 0

ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_TIMER *timer = NULL;

// Fontes
ALLEGRO_FONT *fonteTitulo = NULL;
ALLEGRO_FONT *fonteGrande = NULL;
ALLEGRO_FONT *fontePequena = NULL;

// Imagens
ALLEGRO_BITMAP *arena;
ALLEGRO_BITMAP *obstaculo;
ALLEGRO_BITMAP *fundoGameOver;
ALLEGRO_BITMAP *tanque1, *tanque2;
ALLEGRO_BITMAP *canhao1, *canhao2;
ALLEGRO_BITMAP *projetil;
ALLEGRO_BITMAP *flame[8];

// Sons
ALLEGRO_AUDIO_STREAM *musica = NULL;
ALLEGRO_SAMPLE *atira = NULL;
ALLEGRO_SAMPLE *movimento = NULL;
ALLEGRO_SAMPLE_ID movTanque1, movTanque2;

int contTiros = 0;


#pragma region Constantes

const float FPS = 100;

const int SCREEN_W = 960;
const int SCREEN_H = 540;

const float THETA = ALLEGRO_PI/4;
const float RAIO_CAMPO_FORCA = 50;
const float RAIO_TIRO = 5;

const float VEL_TANQUE = 1.5;
const float PASSO_ANGULO = ALLEGRO_PI/90;

const float VEL_TIRO = 2.0;

int id_tanque = 1;
int vencedor = 0;

#pragma endregion

#pragma region Estruturas

typedef struct Ponto {
	float x, y;
} Ponto;

typedef struct Tiro {
	int ativo;
	Ponto centro;
	Ponto posicao;

	ALLEGRO_COLOR cor;
} Tiro;

typedef struct Tanque {
	float x_comp, y_comp;
	float vel_linear, vel_angular;
	float angulo_movimento;
	int pontos, id;

	Ponto centro, A, B, C;

	ALLEGRO_COLOR cor;
	ALLEGRO_BITMAP *imagem;
} Tanque;

typedef struct Bloco {
	Ponto sup_esq, inf_dir;

	ALLEGRO_COLOR cor;
} Bloco;

typedef struct Botao {
	int x, y, borda_x, borda_y, destacado;

	ALLEGRO_FONT *fonte;
} Botao;

#pragma endregion

#pragma region Cenário

void desenhaCenario() {
	if (DEBUG) {
		al_clear_to_color(al_map_rgb(0, 0, 0));
	} else {
		al_draw_scaled_bitmap(arena, 0, 0, 2550, 3300, 0, 0, SCREEN_W, SCREEN_H, 0);
	}
}

#pragma endregion

#pragma region Menu

void initBotao(Botao *botao, int x, int y, int b_x, int b_y, ALLEGRO_FONT **fonte) {
	botao->x = x;
	botao->y = y;
	botao->borda_x = b_x;
	botao->borda_y = b_y;
	botao->destacado = 0;
	botao->fonte = *fonte;
}

void atualizaBotao(Botao *botao, int x, int y) {

	if (x >= (botao->x - botao->borda_x) && x <= (botao->x + botao->borda_x) && 
	    y >= botao->y && y <= (botao->y + botao->borda_y*2)) {
		botao->destacado = 1;
	} else {
		botao->destacado = 0;
	}
}

void desenhaBotao(Botao botao, char const *nome) {
	if (botao.destacado) {
		al_draw_text(botao.fonte, al_map_rgb(255,255,255), botao.x, botao.y, 1, nome);
	} else {
		al_draw_text(botao.fonte, al_map_rgb(185,185,185), botao.x, botao.y, 1, nome);
	}
}

#pragma endregion

#pragma region Tanque

void desenhaTanque(Tanque t) {
	float angle = t.angulo_movimento+t.vel_angular-ALLEGRO_PI/2;
	
	if (DEBUG) {
		al_draw_circle(t.centro.x, t.centro.y, RAIO_CAMPO_FORCA, t.cor, 1.0);
		al_draw_filled_triangle(t.A.x + t.centro.x, t.A.y + t.centro.y,
								t.B.x + t.centro.x, t.B.y + t.centro.y,
								t.C.x + t.centro.x, t.C.y + t.centro.y,
								t.cor);
	} else {
		if (t.id == 1) {
			al_draw_scaled_rotated_bitmap(t.imagem, al_get_bitmap_width(t.imagem)/2, al_get_bitmap_height(t.imagem)/2,
										  t.centro.x, t.centro.y, 0.5, 0.5, angle, 0);
			al_draw_scaled_rotated_bitmap(canhao1, al_get_bitmap_width(canhao1)/2, al_get_bitmap_height(canhao1)/2,
										  t.centro.x, t.centro.y, 0.5, 0.5, angle, 0);
		} else {
			al_draw_scaled_rotated_bitmap(t.imagem, al_get_bitmap_width(t.imagem)/2, al_get_bitmap_height(t.imagem)/2,
										  t.centro.x, t.centro.y, 0.5, 0.5, angle, 0);
			al_draw_scaled_rotated_bitmap(canhao2, al_get_bitmap_width(canhao2)/2, al_get_bitmap_height(canhao2)/2,
										  t.centro.x, t.centro.y, 0.5, 0.5, angle, 0);
		}
		
	}
}

void rotate(Ponto *p, float angle) {
	float x = p->x, y = p->y;

	p->x = (x * cos(angle)) - (y * sin(angle));
	p->y = (y * cos(angle)) + (x * sin(angle));
}

void initTanque (Tanque *t, char const *imagem) {
	t->imagem = al_load_bitmap(imagem);

	float alpha = ALLEGRO_PI/2 - THETA;
	float h = RAIO_CAMPO_FORCA * sin(alpha);
	float w = RAIO_CAMPO_FORCA * sin(THETA);

	t->id = id_tanque;

	if (t->id == 1) {
		t->centro.x = RAIO_CAMPO_FORCA;
		t->centro.y = SCREEN_H / 2;
	} else {
		t->centro.x = SCREEN_W - RAIO_CAMPO_FORCA;
		t->centro.y = SCREEN_H / 2;
	}
	t->pontos = 0;
	t->cor = al_map_rgb(1 + rand()%254, 1 + rand()%254, 1 + rand()%254);
	t->A.x = 0;
	t->A.y = -RAIO_CAMPO_FORCA;
	t->B.x = -w;
	t->B.y = w;
	t->C.x = w;
	t->C.y = h;
	t->vel_linear = 0;
	t->vel_angular = 0;

	if (t->id == 1) {
		rotate(&t->A, ALLEGRO_PI/2);
		rotate(&t->B, ALLEGRO_PI/2);
		rotate(&t->C, ALLEGRO_PI/2);
		t->angulo_movimento = ALLEGRO_PI;
	} else {
		rotate(&t->A, 3*ALLEGRO_PI/2);
		rotate(&t->B, 3*ALLEGRO_PI/2);
		rotate(&t->C, 3*ALLEGRO_PI/2);
		t->angulo_movimento = 2*ALLEGRO_PI;
	}
	t->x_comp = cos(t->angulo_movimento);
	t->y_comp = sin(t->angulo_movimento);

	id_tanque++;
}

void rotacionaTanque(Tanque *t) {
	if(t->vel_angular != 0) {
		rotate(&t->A, t->vel_angular);
		rotate(&t->B, t->vel_angular);
		rotate(&t->C, t->vel_angular);

		t->angulo_movimento += t->vel_angular;
		t->x_comp = cos(t->angulo_movimento);
		t->y_comp = sin(t->angulo_movimento);

	}
}

void atualizaTanque(Tanque *t) {

	rotacionaTanque(t);
	t->centro.y += t->vel_linear * t->y_comp;
	t->centro.x += t->vel_linear * t->x_comp;

}

#pragma endregion

#pragma region Tiro

void desenhaDisparo(Tiro tiro, Tanque tanque, int indice) {
	float angle = tanque.angulo_movimento+tanque.vel_angular-ALLEGRO_PI/2;
	float posicao_x = al_get_bitmap_width(projetil);
	float posicao_y = al_get_bitmap_height(projetil)*1.5;

	al_draw_scaled_rotated_bitmap(flame[indice], posicao_x, posicao_y,
								  tanque.A.x+tanque.centro.x, tanque.A.y+tanque.centro.y, 0.5, 0.5, angle, 0);

}

void desenhaTiro(Tiro tiro, Tanque t) {
	float angle = t.angulo_movimento+t.vel_angular-ALLEGRO_PI/2;

	if (DEBUG) {
		al_draw_filled_circle(tiro.centro.x, tiro.centro.y, RAIO_TIRO, tiro.cor);
	} else {
		al_draw_scaled_rotated_bitmap(projetil, al_get_bitmap_width(projetil)/2, al_get_bitmap_height(projetil)/2,
									  tiro.centro.x, tiro.centro.y, 0.5, 0.5, angle, 0);
	}
}

void initTiro(Tiro *tiro, Tanque t) {
	tiro->centro.x = t.centro.x + t.A.x;
	tiro->centro.y = t.centro.y + t.A.y;
	tiro->ativo = 0;
	tiro->cor = t.cor;
}

void atualizaTiro(Tiro *tiro) {
	if (tiro->ativo) {
		tiro->centro.x -= tiro->posicao.x;
		tiro->centro.y -= tiro->posicao.y;
	}
}

void disparo(Tanque tanque, Tiro *tiro) {
	tiro->centro.x = tanque.centro.x + tanque.A.x;
	tiro->centro.y = tanque.centro.y + tanque.A.y;
	if (!tiro->ativo) {
		tiro->posicao.x = tanque.x_comp * VEL_TIRO;
		tiro->posicao.y = tanque.y_comp * VEL_TIRO;
		tiro->ativo = 1;
		contTiros++;
		printf("\nConta tiros = %d", contTiros);
	}
}

#pragma endregion

#pragma region Colisões

float distPontos(Ponto p1, Ponto p2) {
	return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

int colisaoCirculoRetangulo(Ponto centro, float raio, Bloco bloco) {
    Ponto p1, p2;

    if (centro.x >= bloco.sup_esq.x && centro.x <= bloco.inf_dir.x) {
        if (centro.y + raio >= bloco.sup_esq.y && centro.y - raio <= bloco.inf_dir.y) {
            return 1;
        }
    }

	if (centro.y >= bloco.sup_esq.y && centro.y <= bloco.inf_dir.y) {
		if (centro.x + raio >= bloco.sup_esq.x && centro.x - raio <= bloco.inf_dir.x) {
			return 1;
		}
	}

    p1.x = bloco.sup_esq.x;
	p1.y = bloco.inf_dir.y;
	p2.x = bloco.inf_dir.x;
	p2.y = bloco.sup_esq.y;

	if (distPontos(centro, bloco.sup_esq) <= raio || distPontos(centro, p1) <= raio ||
		distPontos(centro, bloco.inf_dir) <= raio || distPontos(centro, p2) <= raio) {
			return 1;
	}
	return 0;
}

void colisaoTanqueTanque(Tanque *tanque1, Tanque *tanque2) {
	float raio = 2 * RAIO_CAMPO_FORCA;
	
	if (distPontos(tanque1->centro, tanque2->centro) <= raio) {
		tanque1->centro.y -= tanque1->vel_linear * tanque1->y_comp;
		tanque1->centro.x -= tanque1->vel_linear * tanque1->x_comp;
		tanque2->centro.y -= tanque2->vel_linear * tanque2->y_comp;
		tanque2->centro.x -= tanque2->vel_linear * tanque2->x_comp;
	}
	
}

int colisaoTanqueTiro(Tanque *ataque, Tanque *defesa, Tiro *tiro) {
	Ponto p1, p2;

	p1 = defesa->centro;
	p2 = tiro->centro;
	if (distPontos(p1, p2) <= RAIO_CAMPO_FORCA - RAIO_TIRO) {
		ataque->pontos++;
		tiro->ativo = 0;
		return 1;
	}
	return 0;
}

void colisaoTanqueBloco(Tanque *tanque, Bloco *bloco) {
    int colisao;
	
	colisao = colisaoCirculoRetangulo(tanque->centro, RAIO_CAMPO_FORCA, *bloco);
	
	if (colisao) {
		tanque->centro.y -= tanque->vel_linear * tanque->y_comp;
		tanque->centro.x -= tanque->vel_linear * tanque->x_comp;
	}
}

void colisaoTanqueTela(Tanque *tanque) {
	float altura = al_get_bitmap_height(tanque->imagem)/4;
	if (DEBUG) {
		if (tanque->centro.y <= RAIO_CAMPO_FORCA) {
			tanque->centro.y = RAIO_CAMPO_FORCA;
		}
		if (tanque->centro.y >= SCREEN_H - RAIO_CAMPO_FORCA) {
			tanque->centro.y = SCREEN_H - RAIO_CAMPO_FORCA;
		}
		if (tanque->centro.x <= RAIO_CAMPO_FORCA) {
			tanque->centro.x = RAIO_CAMPO_FORCA;
		}
		if (tanque->centro.x >= SCREEN_W - RAIO_CAMPO_FORCA) {
			tanque->centro.x = SCREEN_W - RAIO_CAMPO_FORCA;
		}
	} else {
		if (tanque->centro.y <= altura) {
			tanque->centro.y = altura;
		}
		if (tanque->centro.y >= SCREEN_H - altura) {
			tanque->centro.y = SCREEN_H - altura;
		}
		if (tanque->centro.x <= altura) {
			tanque->centro.x = altura;
		}
		if (tanque->centro.x >= SCREEN_W - altura) {
			tanque->centro.x = SCREEN_W - altura;
		}
	}
}

void colisaoTiroTela(Tiro *tiro) {
	if (tiro->centro.y <= RAIO_TIRO) {
		tiro->ativo = 0;
	}
	if (tiro->centro.y >= SCREEN_H - RAIO_TIRO) {
		tiro->ativo = 0;
	}
	if (tiro->centro.x <= RAIO_TIRO) {
		tiro->ativo = 0;
	}
	if (tiro->centro.x >= SCREEN_W - RAIO_TIRO) {
		tiro->ativo = 0;
	}
}

void colisaoTiroBloco(Tiro *tiro, Bloco *bloco) {
	int colisao;
	Ponto centroObs;

	colisao = colisaoCirculoRetangulo(tiro->centro, RAIO_TIRO, *bloco);

	if (colisao) {
		tiro->ativo = 0;
	}
}

#pragma endregion

#pragma region Pontuação

void sistemaPontuaco(ALLEGRO_FONT *font, Tanque *tanque) {
	if (tanque->id == 1) {
		al_draw_textf(font, tanque->cor, 50, 50, 0, "PONTOS: %d", tanque->pontos);
	} else {
		al_draw_textf(font, tanque->cor, SCREEN_W-200, 50, 0, "PONTOS: %d", tanque->pontos);
	}
}

#pragma endregion

#pragma region Obstáculo

float randf() {
    return (float)rand() / RAND_MAX;
}

float randFloat(float min, float max) {
    return min + randf() * (max - min);
}

void desenhaBloco(Bloco b) {

	if (DEBUG) {
		al_draw_filled_rectangle(b.sup_esq.x, b.sup_esq.y, b.inf_dir.x, b.inf_dir.y, b.cor);
		// al_draw_rounded_rectangle(b.sup_esq.x, b.sup_esq.y, b.inf_dir.x, b.inf_dir.y, 20, 20, b.cor, 1);
	} else {
		// al_draw_filled_rectangle(b.sup_esq.x, b.sup_esq.y, b.inf_dir.x, b.inf_dir.y, b.cor);
		al_draw_scaled_bitmap(obstaculo, 0, 0, 670, 697, b.sup_esq.x - 40, b.sup_esq.y - 40,
							  b.inf_dir.x-b.sup_esq.x + 70, b.inf_dir.y-b.sup_esq.y + 80, 0);
	}
}

void initBloco(Bloco *b) {
	float x_sup = randFloat(SCREEN_W / 2 - 200, SCREEN_W / 2 - 50);
	float y_sup = randFloat(SCREEN_H / 2 - 150, SCREEN_H / 2 - 50);
	float x_inf = randFloat(SCREEN_W / 2 + 50, SCREEN_W / 2 + 150);
	float y_inf = randFloat(SCREEN_H / 2 + 50, SCREEN_H / 2 + 150);
	b->inf_dir.x = x_inf;
	b->inf_dir.y = y_inf;
	b->sup_esq.x = x_sup;
	b->sup_esq.y = y_sup;
	b->cor = al_map_rgb(rand()%256, rand()%256, rand()%256);
}

#pragma endregion

#pragma region Histórico

int atualizaRegistro(Tanque *tanque1, Tanque *tanque2) {
	FILE *arq, *temp;
	char buf[20], nomeArquivo[] = "historico.txt";
	int jogador1, jogador2, vitoria1, vitoria2;

	arq = fopen(nomeArquivo, "r");
	temp = fopen("temp.txt", "w");
	if (arq == NULL) {
		printf("\nErro ao abrir o arquivo!");
		return -1;
	}
	// Exemplo do arquivo
	// jogador1,vitorias,jogador2,vitorias
	// 1,10,2,5
	fgets(buf, 20, arq);
	jogador1 = atoi(strtok(buf, ","));
	vitoria1 = atoi(strtok(NULL, ","));
	jogador2 = atoi(strtok(NULL, ","));
	vitoria2 = atoi(strtok(NULL, ","));
	fclose(arq);

	if (tanque1->pontos == 5) {
		vencedor = 1;
		vitoria1++;
	}
	if (tanque2->pontos == 5) {
		vencedor = 2;
		vitoria2++;
	}
	tanque1->pontos = vitoria1;
	tanque2->pontos = vitoria2;
	fprintf(temp, "%d,%d,%d,%d\n", jogador1, vitoria1, jogador2, vitoria2);
	fclose(temp);
	remove(nomeArquivo);
	rename("temp.txt", nomeArquivo);
	return vencedor;
}

#pragma endregion

#pragma region Inicialização

void error_msg(char *text) {
	al_show_native_message_box(NULL, "ERRO", "Ocorreu o seguinte erro e o programa sera finalizado:",
							   text, NULL, ALLEGRO_MESSAGEBOX_ERROR);
}

int inicializar() {
	if (!al_init()) {
		error_msg("Falha ao inicializar a Allegro");
		return 0;
	}
	// Sons
	if (!al_install_audio()) {
		error_msg("Falha ao inicializar o audio");
		return 0;
	}

	if(!al_init_acodec_addon()){
        error_msg("Falha ao inicializar o codec de audio");
        return 0;
    }

	if (!al_reserve_samples(3)){
        error_msg("Falha ao reservar amostrar de audio");
        return 0;
    }
	
	atira = al_load_sample("sound/DeathFlash.flac");
	if (!atira) {
		error_msg("Audio nao carregado");
		return 0;
	}

	movimento = al_load_sample("sound/engine_heavy_loop.ogg");
	if (!movimento) {
		error_msg("Audio nao carregado");
		al_destroy_sample(atira);
		return 0;
	}

	/* musica = al_load_audio_stream("sound/soundtrack.ogg", 4, 1024);
	if (!musica) {
		error_msg("Audio nao carregado");
		al_destroy_sample(atira);
		al_destroy_sample(movimento);
		return 0;
	}
	al_attach_audio_stream_to_mixer(musica, al_get_default_mixer());
	al_set_audio_stream_playmode(musica, ALLEGRO_PLAYMODE_LOOP); */

	// Temporizador
    timer = al_create_timer(1.0 / FPS);
    if (!timer) {
		error_msg("Falha ao criar o temporizador");
		return 0;
	}

	// Janela
	display = al_create_display(SCREEN_W, SCREEN_H);
	if (!display) {
		error_msg("Falha ao criar o display");
		al_destroy_timer(timer);
		al_destroy_audio_stream(musica);
		return 0;
	}
	al_set_window_title(display, "Combat");
	
	if (!al_install_keyboard()) {
		error_msg("Falha ao instalar o teclado");
		return 0;
	}

	if (!al_install_mouse()) {
		error_msg("Falha ao instalar o mouse");
		al_destroy_timer(timer);
		al_destroy_display(display);
		return 0;
	}

	event_queue = al_create_event_queue();
	if (!event_queue) {
		error_msg("Falha ao criar a fila de eventos");
		al_destroy_timer(timer);
		al_destroy_display(display);
		return 0;
	}

	// Fontes
	al_init_font_addon();
	if (!al_init_ttf_addon()) {
		error_msg("Falha ao carregar o modulo de fonte tff");
		return 0;
	}

	fonteTitulo = al_load_font("fonts/REVOLUTI.TTF", 72, 0);
	fonteGrande = al_load_font("fonts/REVOLUTI.TTF", 32, 0);   
	fontePequena = al_load_font("fonts/REVOLUTI.TTF", 16, 0);
	if (!fonteTitulo || !fonteGrande || !fontePequena) {
		error_msg("Falha ao carregar as fontes");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	// Desenho
	if (!al_init_primitives_addon()) {
		error_msg("Falha ao inicializar o modulo de primitivas");
        return 0;
    }

	// Imagens
	if (!al_init_image_addon()) {
		error_msg("Falha ao inicializar o modulo de imagens");
		return 0;
	}

	arena = al_load_bitmap("img/Ground.jpg");
	if (!arena) {
		error_msg("Falha ao carregar arena");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	obstaculo = al_load_bitmap("img/Landscape_Plot_4B.png");
	if (!obstaculo) {
		error_msg("Falha ao carregar obstaculo");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	fundoGameOver = al_load_bitmap("img/gameover2.jpg");
	if (!fundoGameOver) {
		error_msg("Falha ao carregar fundo");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	canhao1 = al_load_bitmap("img/Gun_01.png");
	canhao2 = al_load_bitmap("img/Gun_02.png");
	if (!canhao1 || !canhao2) {
		error_msg("Falha ao carregar canhoes");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	projetil = al_load_bitmap("img/Light_Shell.png");
	if (!projetil) {
		error_msg("Falha ao carregar projetil");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	flame[0] = al_load_bitmap("img/Flame_A.png");
	flame[1] = al_load_bitmap("img/Flame_B.png");
	flame[2] = al_load_bitmap("img/Flame_C.png");
	flame[3] = al_load_bitmap("img/Flame_D.png");
	flame[4] = al_load_bitmap("img/Flame_E.png");
	flame[5] = al_load_bitmap("img/Flame_F.png");
	flame[6] = al_load_bitmap("img/Flame_G.png");
	flame[7] = al_load_bitmap("img/Flame_H.png");
	if (!flame[0] || !flame[1] || !flame[2]) {
		error_msg("Falha ao carregar imagem do disparo");
		al_destroy_timer(timer);
		al_destroy_display(display);
		al_destroy_event_queue(event_queue);
		return 0;
	}

	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_mouse_event_source());

	return 1;
}

#pragma endregion

int main(int argc, char **argv) {
	// Instanciação dos objetos
	Tanque tanque_1, tanque_2;
	Bloco bloco_1;
	Tiro tiro_1, tiro_2;
	Botao bMenu, bContinuar, bNovaTentativa;
	ALLEGRO_COLOR corTexto = al_map_rgb(255,255,255);
		
	int sair = 0, pause = 0, ehGameOver = 0, idVencedor = 0, i = 0;

	// Posição do Mouse
	int mouseX;
	int mouseY;

	// quantos frames devem se passar para atualizar para o proximo sprite
    int frames_sprite = 6, cont_frames = 0, img_atual = 0, img_total = 8, disparo1 = 0, disparo2 = 0;

	srand(time(NULL));
   
	if (!inicializar()) {
		return -1;
	}

	initTanque(&tanque_1, "img/Hull_01.png");	
	initTanque(&tanque_2, "img/Hull_02.png");


	initBloco(&bloco_1);

	initBotao(&bMenu, SCREEN_W/2, SCREEN_H/2 + 200, 60, 18, &fonteGrande);
	initBotao(&bContinuar, SCREEN_W/2, SCREEN_H/2 + 140, 140, 18, &fonteGrande);
	initBotao(&bNovaTentativa, SCREEN_W/2, SCREEN_H/2 + 140, 205, 18, &fonteGrande);

	al_start_timer(timer);

	while(!sair) {
		ALLEGRO_EVENT ev;

		al_wait_for_event(event_queue, &ev);

		if(ev.type == ALLEGRO_EVENT_TIMER) {
			
			cont_frames++;
			if (cont_frames >= frames_sprite) {
				cont_frames = 0;
				img_atual++;
				if (img_atual >= img_total) {
					img_atual = 0;
				}
			}
			if (pause) {
				al_draw_textf(fonteTitulo, al_map_rgb(255,255,255),
				 			  SCREEN_W/2, SCREEN_H/2-200, ALLEGRO_ALIGN_CENTRE, "PAUSE");

				atualizaBotao(&bContinuar, mouseX, mouseY);
				atualizaBotao(&bMenu, mouseX, mouseY);

				desenhaBotao(bContinuar, "Continuar");
				desenhaBotao(bMenu, "SAIR");

			}

			if (ehGameOver) {
				atualizaBotao(&bNovaTentativa, mouseX, mouseY);
				atualizaBotao(&bMenu, mouseX, mouseY);

				al_draw_scaled_bitmap(fundoGameOver, 0, 0, 1920, 1080, 0, 0, SCREEN_W, SCREEN_H, 0);
				al_draw_textf(fonteTitulo, al_map_rgb(255,255,255),
							SCREEN_W/2, SCREEN_H/2-200, ALLEGRO_ALIGN_CENTRE, "GAME OVER");
				idVencedor = atualizaRegistro(&tanque_1, &tanque_2);
				al_draw_textf(fonteGrande, al_map_rgb(255,255,255), SCREEN_W/2, SCREEN_H/2-100, ALLEGRO_ALIGN_CENTRE,
							  "VENCEDOR PLAYER %d", idVencedor);
				al_draw_textf(fontePequena, al_map_rgb(255,255,255),
							SCREEN_W/2, SCREEN_H/2, ALLEGRO_ALIGN_CENTRE, "Player 1: %d vitorias", tanque_1.pontos);
				al_draw_textf(fontePequena, al_map_rgb(255,255,255),
							SCREEN_W/2, SCREEN_H/2+50, ALLEGRO_ALIGN_CENTRE, "Player 2: %d vitorias", tanque_2.pontos);
				
				desenhaBotao(bNovaTentativa, "Jogar de Novo");
				desenhaBotao(bMenu, "Sair");
			} 
			if (!ehGameOver && !pause) {
				
				desenhaCenario();

				desenhaBloco(bloco_1);
				
				atualizaTanque(&tanque_1);
				atualizaTanque(&tanque_2);

				atualizaTiro(&tiro_1);
				atualizaTiro(&tiro_2);

				if (!tiro_1.ativo) {
					initTiro(&tiro_1, tanque_1);
					disparo1 = 0;
				}
				if (!tiro_2.ativo) {
					initTiro(&tiro_2, tanque_2);
					disparo2 = 0;
				}
				
				colisaoTanqueTela(&tanque_1);
				colisaoTanqueTela(&tanque_2);

				colisaoTiroTela(&tiro_1);
				colisaoTiroTela(&tiro_2);

				colisaoTiroBloco(&tiro_1, &bloco_1);
				colisaoTiroBloco(&tiro_2, &bloco_1);

				if (colisaoTanqueTiro(&tanque_1, &tanque_2, &tiro_1) && contTiros % 3 == 0) {
					ehGameOver = 1;
				}

				colisaoTanqueTiro(&tanque_2, &tanque_1, &tiro_2);

				colisaoTanqueBloco(&tanque_1, &bloco_1);
				colisaoTanqueBloco(&tanque_2, &bloco_1);

				colisaoTanqueTanque(&tanque_1, &tanque_2);

				desenhaTanque(tanque_1);
				desenhaTanque(tanque_2);

				
				if (tanque_1.pontos == 5 || tanque_2.pontos == 5) {
					ehGameOver = 1;	
				}

				desenhaTiro(tiro_1, tanque_1);
				desenhaTiro(tiro_2, tanque_2);

				if (!DEBUG) {

					if (tiro_1.ativo && disparo1 == img_atual) {
						disparo1++;
						desenhaDisparo(tiro_1, tanque_1, img_atual);
					}
					if (tiro_2.ativo && disparo2 == img_atual) {
						disparo2++;
						desenhaDisparo(tiro_2, tanque_2, img_atual);
					}
				}
				
				sistemaPontuaco(fontePequena, &tanque_1);
				sistemaPontuaco(fontePequena, &tanque_2);
			}
			
			al_flip_display();
			
			if(al_get_timer_count(timer)%(int)FPS == 0)
				printf("\n%d segundos se passaram...", (int)(al_get_timer_count(timer)/FPS));
		}
		else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			sair = 1;
		}
		else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
			switch(ev.keyboard.keycode) {
				#pragma region Jogador1
				case ALLEGRO_KEY_ESCAPE:
					if (tanque_1.pontos < 5 || tanque_2.pontos < 5) {
						if (!pause) {
							pause = 1;
						} else {
							pause = 0;
						}
					}
					break;
				case ALLEGRO_KEY_W:
					tanque_1.vel_linear -= VEL_TANQUE;
					if (!pause && !DEBUG) {
						al_play_sample(movimento, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &movTanque1);
					}
					break;
				case ALLEGRO_KEY_S:
					tanque_1.vel_linear += VEL_TANQUE;
					if (!pause && !DEBUG) {
						al_play_sample(movimento, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &movTanque1);
					}
					break;
				case ALLEGRO_KEY_D:
					tanque_1.vel_angular += PASSO_ANGULO;
					break;
				case ALLEGRO_KEY_A:
					tanque_1.vel_angular -= PASSO_ANGULO;
					break;
				case ALLEGRO_KEY_Q:
					if (!tiro_1.ativo && !pause) {
						disparo(tanque_1, &tiro_1);
						if (!DEBUG) {
							al_play_sample(atira, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
						}
					}
					break;
				#pragma endregion
				#pragma region Jogador2
				case ALLEGRO_KEY_UP:
					tanque_2.vel_linear -= VEL_TANQUE;
					if (!pause && !DEBUG) {
						al_play_sample(movimento, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &movTanque2);
					}
					break;
				case ALLEGRO_KEY_DOWN:
					tanque_2.vel_linear += VEL_TANQUE;
					if (!pause && !DEBUG) {
						al_play_sample(movimento, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, &movTanque2);
					}
					break;
				case ALLEGRO_KEY_RIGHT:
					tanque_2.vel_angular += PASSO_ANGULO;
					break;
				case ALLEGRO_KEY_LEFT:
					tanque_2.vel_angular -= PASSO_ANGULO;
					break;
				case ALLEGRO_KEY_ENTER:
					if (!tiro_2.ativo && !pause) {
						disparo(tanque_2, &tiro_2);
						if (!DEBUG) {
							al_play_sample(atira, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
						}
					}
					break;
				#pragma endregion
			}
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
			switch(ev.keyboard.keycode) {
				#pragma region Jogador1
				case ALLEGRO_KEY_W:
					tanque_1.vel_linear += VEL_TANQUE;
					al_stop_sample(&movTanque1);
					break;
				case ALLEGRO_KEY_S:
					tanque_1.vel_linear -= VEL_TANQUE;
					al_stop_sample(&movTanque1);
					break;
				case ALLEGRO_KEY_D:
					tanque_1.vel_angular -= PASSO_ANGULO;
					break;
				case ALLEGRO_KEY_A:
					tanque_1.vel_angular += PASSO_ANGULO;
					break;
				#pragma endregion
				#pragma region Jogador2
				// Teclas de ação do Jogador 2
				case ALLEGRO_KEY_UP:
					tanque_2.vel_linear += VEL_TANQUE;
					al_stop_sample(&movTanque2);
					break;
				case ALLEGRO_KEY_DOWN:
					tanque_2.vel_linear -= VEL_TANQUE;
					al_stop_sample(&movTanque2);
					break;
				case ALLEGRO_KEY_RIGHT:
					tanque_2.vel_angular -= PASSO_ANGULO;
					break;
				case ALLEGRO_KEY_LEFT:
					tanque_2.vel_angular += PASSO_ANGULO;
					break;
				#pragma endregion
			}
		}
		else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			mouseX = ev.mouse.x;
			mouseY = ev.mouse.y;
		}
		else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if (pause) {
				if(bMenu.destacado) {
					sair = 1;
				} else if (bContinuar.destacado) {
					pause = 0;
				}
			} else if (ehGameOver) {
				if (bMenu.destacado) {
					sair = 1;
				} else if (bNovaTentativa.destacado) {
					id_tanque = 1;
					vencedor = 0;
					ehGameOver = 0;
					initTanque(&tanque_1, "img/Hull_01.png");	
					initTanque(&tanque_2, "img/Hull_02.png");
					contTiros = 0;
					initBloco(&bloco_1);
				}
			}
		}
	} //fim do while

	// Desaloca memoria dos sons
	al_destroy_sample(atira);
	al_destroy_sample(movimento);
	al_destroy_audio_stream(musica);

	// Desaloca memoria das imagens
	al_destroy_bitmap(arena);
	al_destroy_bitmap(obstaculo);
	al_destroy_bitmap(fundoGameOver);
	al_destroy_bitmap(tanque1);
	al_destroy_bitmap(tanque2);
	al_destroy_bitmap(canhao1);
	al_destroy_bitmap(canhao2);
	al_destroy_bitmap(projetil);
	for (i = 0; i < 8; i++) {
		al_destroy_bitmap(flame[i]);
	}

	// Desaloca memoria das fontes
	al_destroy_font(fonteTitulo);
	al_destroy_font(fonteGrande);
	al_destroy_font(fontePequena);

	// Desaloca memoria do timer
	al_destroy_timer(timer);

	// Desaolica memoria da lista de eventos
	al_destroy_event_queue(event_queue);

	// Desaloca memoria da janela
	al_destroy_display(display);
 
	return 0;
}