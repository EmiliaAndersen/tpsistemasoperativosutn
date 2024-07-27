#ifndef MEMORIA_UTILS_H
#define MEMORIA_UTILS_H
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <utils/catedra/inicio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "main.h"
#include <math.h>
#include <utils/funcionesUtiles.h>

extern int retardo;
extern int tam_pagina;
extern int tam_memoria;
extern t_list* tabla_pags;
extern void* memoria;
extern t_bitarray* bitarray;
//extern t_bitarray* escrito;
extern pthread_mutex_t mutex_tabla_pags;


int buscar_frame_disp(t_bitarray* bit, int tam);
bool buscar_por_pid_bool(uint32_t pid);
t_tabla* buscar_por_pid_return(uint32_t pid); 
void escribir_en_mem_cpu(char* aescribir, t_dir_fisica* dir_fisica, int tamanio , uint32_t pid);
void escribir_en_mem_io(char* aescribir, t_dir_fisica* dir_fisica, int cant_direcciones,int tamanio, uint32_t pid );
char* leer_en_mem_cpu(int tamanio, t_dir_fisica* dir_fisica, uint32_t pid);
char* leer_en_mem_io(int tamanio, t_dir_fisica* dir_fisica, int cant_direcciones, uint32_t pid); 
int frame_sig_disp( uint32_t pid, int frame );

int frame_sig_leer( uint32_t pid, int frame);
bool puede_escribir(uint32_t pid, int frame,int cant_pags );
char* decstring(const char* str, int start, int end);

t_tabla* buscar_por_pid(uint32_t pid);
t_tabla* eliminar_tabla_pid(uint32_t pid);
#endif