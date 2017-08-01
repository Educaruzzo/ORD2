#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TAM_MAX_BUCKET 3  // Tamanho do BUCKET (A DEFINIR)

#define TAM_IDNUM 4     // sizeof(int)
#define TAM_DIRPROF 4   // sizeof(int)

typedef struct bucket {
    int prof;
    int cont;
    int chaves[TAM_MAX_BUCKET];
    int id;
} BUCKET;

typedef struct dir_cell {
    BUCKET *bucket_ref;
} DIR_CELL;

typedef struct diret {
    int profundidade;
    DIR_CELL *celulas;
} DIRETORIO;


int ID_NUM = 1;

DIRETORIO diretorio = {0, NULL};



void RecuperaDiretorio(FILE *arqdir) {   // Formato do arqdir: #ID_NUM#profundidade#dircell1#dircell2...

    int i, tam_dir;

    fseek(arqdir, 0, SEEK_SET);

    fread(&ID_NUM, TAM_IDNUM, 1, arqdir);
    fread(&diretorio.profundidade, TAM_DIRPROF, 1, arqdir);

    tam_dir = (int) pow(2, diretorio.profundidade);

    diretorio.celulas = (DIR_CELL*) malloc (tam_dir*sizeof(DIR_CELL));

    for (i = 0; i < tam_dir; i++) {
        fread(&diretorio.celulas[i].bucket_ref, sizeof(BUCKET*), 1, arqdir);
    }
}


int make_address(int KEY, int PROF) {
    int RETVAL, MASK, LOW_BIT, HASHVAL, j;

    RETVAL = 0;
    MASK = 1;
    HASHVAL = KEY;
    for (j = 1; j <= PROF; j++) {
        RETVAL = RETVAL << 1;
        LOW_BIT = HASHVAL & MASK;
        RETVAL = RETVAL | LOW_BIT;
        HASHVAL = HASHVAL >> 1;
    }

    return RETVAL;
}


/*BUCKET* find_bucket_address(int ADDRESS) {

    int i;
    DIR_CELL *celula = diretorio.celulas;

    for (i = 0; i<ADDRESS; i++) {
        celula = celula->proxcell;
    }

    return celula->bucket_ref;
}*/

void dir_double() {

    DIRETORIO NOVO_DIR;
    int TAM_ATUAL, NOVO_TAM, i;
    DIR_CELL *pntCelula;

    TAM_ATUAL = (int) pow(2.0, (double) diretorio.profundidade);
    NOVO_TAM = 2*TAM_ATUAL;
    
    pntCelula = (DIR_CELL*) malloc(NOVO_TAM*sizeof(DIR_CELL));
    //InsereDiretorio(&NOVO_DIR.celulas, pntCelula);
    
    for (i = 0; i < TAM_ATUAL; i++) {
        pntCelula[2*i].bucket_ref = diretorio.celulas[i].bucket_ref;
        pntCelula[2*i + 1].bucket_ref = diretorio.celulas[i].bucket_ref;
    }
    free(diretorio.celulas);
    diretorio.celulas = pntCelula;

    diretorio.profundidade = diretorio.profundidade + 1;
}

void find_new_range (BUCKET OLD_BUCKET, int *NEW_START, int *NEW_END) {

    int MASK, SHARED_ADDRESS, NEW_SHARED, BITS_TO_FILL, j;

    MASK = 1;
    SHARED_ADDRESS = makeadress(OLD_BUCKET.chaves[0], OLD_BUCKET.prof);
    NEW_SHARED = SHARED_ADDRESS << 1;
    NEW_SHARED = NEW_SHARED | MASK;
    BITS_TO_FILL = diretorio.profundidade - (OLD_BUCKET.prof + 1);
    (*NEW_START) = (*NEW_END) = NEW_SHARED;

    for (j = 0; j < BITS_TO_FILL; j++) {
        (*NEW_START) = (*NEW_START) << 1;
        (*NEW_END) = (*NEW_END) << 1;
        (*NEW_END) = (*NEW_END) | MASK;
    }
}

void dir_ins_bucket (BUCKET* BUCKET_ADDRESS, int START, int END) {

    int j;

    for (j = START; j <= END; j++) {
        diretorio.celulas[j].bucket_ref = BUCKET_ADDRESS;
    }
}

void bk_split (BUCKET *Buck) {

    BUCKET *END_NOVO_BUCKET = (BUCKET*) malloc(sizeof(BUCKET));

    int NEW_START, NEW_END, i, j;
    int guardakeys[TAM_MAX_BUCKET];

    if ((*Buck).prof == diretorio.profundidade) {
        dir_double();
    }

    find_new_range((*Buck), &NEW_START, &NEW_END);
    dir_ins_bucket(END_NOVO_BUCKET, NEW_START, NEW_END);

    (*Buck).prof = (*Buck).prof + 1;
    (*END_NOVO_BUCKET).prof = (*Buck).prof;
    (*END_NOVO_BUCKET).cont = 0;
    (*END_NOVO_BUCKET).id = ID_NUM;
    ID_NUM++;

    for(i = 0; i < (*Buck).cont; i++) {
        guardakeys[i] = (*Buck).chaves[i];
    }
    (*Buck).cont = 0;

    for(j = 0; j < i; j++) {
        op_add(guardakeys[j]);
    }
}

void bk_add_key (BUCKET *Buck, int KEY) {
    if ((*Buck).cont < TAM_MAX_BUCKET) {
        (*Buck).chaves[(*Buck).cont] = KEY;
        (*Buck).cont++;
    }
    else {
        bk_split(Buck);
        op_add(KEY);
    }
}

int op_find (int KEY, BUCKET **FOUND_BUCKET) {

    int ADDRESS = make_address(KEY, diretorio.profundidade);
    int j;

    (*FOUND_BUCKET) = diretorio.celulas[ADDRESS].bucket_ref;

    for (j = 0; j < (*FOUND_BUCKET)->cont; j++) {
        if ((*FOUND_BUCKET)->chaves[j] == KEY) {
            return 1;
        }
    }

    return 0;
}


int op_add (int KEY) {

    BUCKET *FOUND_BUCKET;

    if (!(op_find(KEY, &FOUND_BUCKET))) {
        bk_add_key(FOUND_BUCKET, KEY);
        return 1;
    }

    return 0;
}


char* get_key(FILE *arq) {

    char *num = (char*) malloc(100);
    int i = 0;
    char c;

    c = fgetc(arq);
    while (c != '\n') {
        num[i] = c;
        i++;
        fgetc(arq);
    }
    num[i] = '\0';

    return num;
}



int main () {

    FILE *arq = fopen("chaves.txt", "r");
    FILE *arqdir = fopen("diretorio.txt", "r+");
    char numero[100];

    if (arqdir == NULL) {
        arqdir = fopen("diretorio.txt", "w");
        diretorio.celulas = (DIR_CELL*) malloc(sizeof(DIR_CELL));
        diretorio.celulas[0].bucket_ref = NULL;
    }
    else {
        RecuperaDiretorio(arqdir);
    }


    while (!feof(arq)) {
        op_add(atoi(get_key(arq)));
    }

    ExibeBuckets();
    RegistraDiretorio(arqdir);

    fclose(arqdir);
    fclose(arq);

    return 0;
}
