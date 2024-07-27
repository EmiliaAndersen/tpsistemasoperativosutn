#include "ciclo_inst.h"

char* fetch(int conexion, t_pcb* pcb){
    
    enviar_pid(int_to_char(pcb->pid), conexion);
    enviar_pc(int_to_char(pcb->registros->PC),conexion);
    int x = recibir_operacion(conexion);
    char* ins = recibir_mensaje(conexion, cpu_log);
    pcb->registros->PC++;
    return ins;
}

char* eliminar_linea_n(char* instruccion) {
    int len = strlen(instruccion);
    if (len > 0 && instruccion[len - 1] == '\n') {
        instruccion[len - 1] = '\0';
    }
    return instruccion;
}

instrucciones obtener_instruccion(char *nombre) {
    if (strcmp(nombre, "SET") == 0) 
    {
        return SET;
    }
    if (strcmp(nombre, "MOV_IN") == 0) 
    {
        return MOV_IN;
    }
    if (strcmp(nombre, "MOV_OUT") == 0) 
    {
        return MOV_OUT;
    } 
    if (strcmp(nombre, "SUM") == 0) {
        return SUM;
    } 
    if (strcmp(nombre, "SUB") == 0) {
        return SUB;
    } 
    if (strcmp(nombre, "JNZ") == 0) {
        return JNZ;
    } 
    if (strcmp(nombre, "RESIZE") == 0) {
        return RESIZE;
    } 
    if (strcmp(nombre, "COPY_STRING") == 0) {
        return COPY_STRING;
    } 
    if (strcmp(nombre, "WAIT") == 0) {
        return WAIT;
    } 
    if (strcmp(nombre, "SIGNAL") == 0) {
        return SIGNAL;
    } 
    if (strcmp(nombre, "IO_GEN_SLEEP") == 0) {
        return IO_GEN_SLEEP;
    } 
    if (strcmp(nombre, "IO_STDIN_READ") == 0) {
        return IO_STDIN_READ;
    } 
    if (strcmp(nombre, "IO_STDOUT_WRITE") == 0) {
        return IO_STDOUT_WRITE;
    } 
    if (strcmp(nombre, "IO_FS_CREATE") == 0) {
        return IO_FS_CREATE;
    } 
    if (strcmp(nombre, "IO_FS_DELETE") == 0) {
        return IO_FS_DELETE;
    } 
    if (strcmp(nombre, "IO_FS_TRUNCATE") == 0) {
        return IO_FS_TRUNCATE;
    } 
    if (strcmp(nombre, "IO_FS_WRITE") == 0) {
        return IO_FS_WRITE;
    } 
    if (strcmp(nombre, "IO_FS_READ") == 0) {
        return IO_FS_READ;
    } 
    if (strcmp(nombre, "EXIT") == 0) {
        return EXIIT;
    } 
    return EXIIT;
}

t_decode* decode(char* buffer){

    eliminar_linea_n(buffer);
    char** arrayIns = string_split(buffer," ");

    instrucciones ins = obtener_instruccion(arrayIns[0]);
    t_decode* decode = malloc(sizeof(t_decode));
    decode->op_code = ins;
    decode->registroCpu = list_create();
    
    switch(ins){
        case SET:{
        char* registro = strdup(arrayIns[1]);
        list_add(decode->registroCpu, registro);
        int valor =atoi(strdup(arrayIns[2]));
        decode->valor = valor;     
        break;     
        }
        case MOV_IN:{
        char* registroDatos = strdup(arrayIns[1]);
        list_add(decode->registroCpu,registroDatos);
        char* registroDireccion = strdup(arrayIns[2]);
        list_add(decode->registroCpu,registroDireccion);     
        break;
        }
        case MOV_OUT:{
        char* registroDireccion = strdup(arrayIns[1]);
        list_add(decode->registroCpu,registroDireccion);
        char* registroDatos = strdup(arrayIns[2]);
        list_add(decode->registroCpu,registroDatos);   
        decode->logicaAFisica= true;  
        break;
        }
        case SUM:{
        char* registroDestino = strdup(arrayIns[1]);
        list_add(decode->registroCpu,registroDestino);
        char* registroOrigen= strdup(arrayIns[2]);
        list_add(decode->registroCpu,registroOrigen);     
        break;
        }
        case SUB:{
        char* registroDestino = strdup(arrayIns[1]);
        list_add(decode->registroCpu,registroDestino);
        char* registroOrigen= strdup(arrayIns[2]);
        list_add(decode->registroCpu,registroOrigen);
        break;
        }
        case JNZ:{
        char* registro=strdup(arrayIns[1]);
        list_add(decode->registroCpu,registro);
        instrucciones ins = atoi(strdup(arrayIns[2]));
        decode->instrucciones= ins;
        break;
        }
        case RESIZE:{
        int tamanio = atoi(strdup(arrayIns[1]));
        decode->valor= tamanio;
        break;
        }   
        case COPY_STRING:{
        int tamanio = atoi(strdup(arrayIns[1]));
        decode->valor= tamanio;
        break;
        }
        case WAIT:{
        char *rec;
        rec= strdup(arrayIns[1]);
        decode->recurso = rec;
        break;
        }
        case SIGNAL:{
        char* recu = strdup(arrayIns[1]);
        decode->recurso=recu;
        break;
        }
        case IO_GEN_SLEEP:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        int unidadesTrabajo = atoi(strdup(arrayIns[2]));
        decode->valor = unidadesTrabajo;
        break;
        }
        case IO_STDIN_READ:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* registro_direccion = strdup(arrayIns[2]);
        list_add(decode->registroCpu, registro_direccion);
        char* registro_tamanio = strdup(arrayIns[3]);
        list_add(decode->registroCpu, registro_tamanio);
        break;
        }
        case IO_STDOUT_WRITE:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* registro_direccion = strdup(arrayIns[2]);
        list_add(decode->registroCpu, registro_direccion);
        char* registro_tamanio = strdup(arrayIns[3]);
        list_add(decode->registroCpu, registro_tamanio);  
        break;
        }
        case IO_FS_CREATE:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* archivo = strdup(arrayIns[2]);
        decode->archivo = archivo;
        break;
        }
        case IO_FS_DELETE:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* archivo = strdup(arrayIns[2]);
        decode->archivo = archivo;
        break;
        }
        case IO_FS_TRUNCATE:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* archivo = strdup(arrayIns[2]);
        decode->archivo = archivo;
        char* registroTamanio = strdup(arrayIns[3]);
        list_add(decode->registroCpu, registroTamanio);
        break;
        }
        case IO_FS_WRITE:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* archivo = strdup(arrayIns[2]);
        decode->archivo = archivo;
        char* registroDireccion = strdup(arrayIns[3]);
        list_add(decode->registroCpu, registroDireccion);
        char* registroTamanio = strdup(arrayIns[4]);
        list_add(decode->registroCpu, registroTamanio);
        char* registroPuntero = strdup(arrayIns[5]);
        list_add(decode->registroCpu, registroPuntero);
        break;
        }
        case IO_FS_READ:{
        char* interfaz = strdup(arrayIns[1]);
        decode->interfaz = interfaz;
        char* archivo = strdup(arrayIns[2]);
        decode->archivo = archivo;
        char* registroDireccion = strdup(arrayIns[3]);
        list_add(decode->registroCpu, registroDireccion);
        char* registroTamanio = strdup(arrayIns[4]);
        list_add(decode->registroCpu, registroTamanio);
        char* registroPuntero = strdup(arrayIns[5]);
        list_add(decode->registroCpu, registroPuntero);
        break;
        }
        case EXIIT:{
            break;
        }

    }
    for (int i = 0; arrayIns[i] != NULL; i++) {
    free(arrayIns[i]);
    }
    free(arrayIns);
    return decode;
}

void asignar_registro(cpu_registros* regs, const char* nombre_registro, void* valor) {
    if (strcmp(nombre_registro, "PC") == 0)  {regs->PC=valor;}
    else if (strcmp(nombre_registro, "AX") == 0)  {regs->AX=valor;}
    else if (strcmp(nombre_registro, "BX") == 0)  {regs->BX=valor;}
    else if (strcmp(nombre_registro, "CX") == 0)  {regs->CX=valor;}
    else if (strcmp(nombre_registro, "DX") == 0)  {regs->DX=valor;}
    else if (strcmp(nombre_registro, "EAX") == 0)  {regs->EAX=valor;}
    else if (strcmp(nombre_registro, "EBX") == 0)  {regs->EBX=valor;}
    else if (strcmp(nombre_registro, "ECX") == 0) {regs->ECX=valor;}
    else if (strcmp(nombre_registro, "EDX") == 0)  {regs->EDX=valor;}
    else if (strcmp(nombre_registro, "SI") == 0)  {regs->SI=valor;}
    else if (strcmp(nombre_registro, "DI") == 0)  {regs->DI=valor;}
}

void* obtener_valor_registro(cpu_registros* regs, char* nombre_registro) {
    if (strcmp(nombre_registro, "PC") == 0) {return regs->PC;}
    else if (strcmp(nombre_registro, "AX") == 0) {return regs->AX;}
    else if (strcmp(nombre_registro, "BX") == 0) {return regs->BX;}
    else if (strcmp(nombre_registro, "CX") == 0) {return  regs->CX;}
    else if (strcmp(nombre_registro, "DX") == 0) {return regs->DX;}
    else if (strcmp(nombre_registro, "EAX") == 0) {return regs->EAX;}
    else if (strcmp(nombre_registro, "EBX") == 0) {return regs->EBX;}
    else if (strcmp(nombre_registro, "ECX") == 0) {return regs->ECX;}
    else if (strcmp(nombre_registro, "EDX") == 0) {return regs->EDX;}
    else if (strcmp(nombre_registro, "SI") == 0) {return regs->SI;}
    else if (strcmp(nombre_registro, "DI") == 0) {return regs->DI;}
    else {return NULL;}
}

int obtener_tipo_registro(char* nombre_registro){
    if (strcmp(nombre_registro, "PC") == 0 || strcmp(nombre_registro, "AX") == 0 || 
        strcmp(nombre_registro, "BX") == 0 || strcmp(nombre_registro, "CX") == 0 || 
        strcmp(nombre_registro, "DX") == 0) {
        return 8;
    } else {
        return 32;
    }
}

t_cpu_blockeo execute(t_decode* decode, t_pcb* pcb, t_log *logger){
    
    instrucciones ins = decode->op_code;
    t_cpu_blockeo ret;
    ret.blockeo = NO_BLOCK;
    ret.instrucciones = NULL;
    switch(ins){
    
        case SET:{
            uint8_t valor = (uint8_t) decode->valor;
            char* registro_adepositar = list_get(decode->registroCpu,0);
            log_info(logger, "PID: <%d> - Ejecutando: SET - <%s %u>", pcb->pid, registro_adepositar, valor);        
            asignar_registro(pcb->registros, registro_adepositar, valor);    
            break;
         }
        case MOV_IN:{
            char* registro_datos = list_get(decode->registroCpu,0);
            char* registro_direccion = list_get(decode->registroCpu,1);
            uint8_t dir_logica = (uint8_t) obtener_valor_registro(pcb->registros, registro_direccion);
            t_dir_fisica* dir_fisica = mmu(dir_logica, pcb->pid);
            
            log_info(logger, "PID: <%u> - Ejecutando: <MOV_IN> - <%s %s>", pcb->pid, registro_datos, registro_direccion);        

            int tamanio;

            if(obtener_tipo_registro(registro_datos) == 8){
                tamanio =1;
            }
            else{
                tamanio = 4;
            }

            int num_frame = dir_fisica->nro_frame;
            int desplazamiento = dir_fisica->desplazamiento;
            uint32_t pid=pcb->pid;


            int cant_pags =1;
            char* frame_siguiente;
            int tam_primera=tam_pag - desplazamiento;
            char* leido =malloc(tamanio);
            leido= string_new();
            if(tam_primera<tamanio){
                cant_pags++;
            }
            if(cant_pags==1){
            int tam_mensaje = sizeof(tamanio)+sizeof(num_frame)+sizeof(desplazamiento)+sizeof(pid); 
            char* mensaje = malloc(tam_mensaje);
            sprintf(mensaje, "%d/%d/%d/%u", tamanio,num_frame,desplazamiento,pid); 

            enviar_a_mem(conexion_memoria_cpu, mensaje, PED_LECTURA);


            int i = recibir_operacion(conexion_memoria_cpu);
            char* msj = recibir_mensaje(conexion_memoria_cpu, cpu_log);
            int fra;
            int pag;
            sscanf(msj, "%s/%d/%d", leido,&fra,&pag);
            log_info(logger, "PID: <%u> - Accion: <LEER> - Direccion Fisica: <%d> - Valor: <%s>", pid ,num_frame+desplazamiento, leido); 
            asignar_registro(pcb->registros, registro_datos, atoi(leido));
            }else{
            int tam_mensaje1 = sizeof(uint16_t)+sizeof(num_frame)+sizeof(desplazamiento)+sizeof(pid); 
            char* mensaje1 = malloc(tam_mensaje1);
            sprintf(mensaje1, "%d/%d/%d/%u", 2,num_frame,desplazamiento,pid); 

            enviar_a_mem(conexion_memoria_cpu, mensaje1, PED_LECTURA);

            int i = recibir_operacion(conexion_memoria_cpu);
            char* msj = recibir_pedido(conexion_memoria_cpu);
            char leo[8];
            int fra_nuevo;
            int pag1;
            sscanf(msj, "%7[^/]/%d/%d", leo,&fra_nuevo,&pag1);
            t_dir_fisica* dir_fisica = mmu(pag1*tam_pag, pcb->pid);
            int num_frame = dir_fisica->nro_frame;
            
            log_info(logger, "PID: <%u> - Accion: <LEER> - Direccion Fisica: <%d> - Valor: <%s>", pid ,fra_nuevo, leo); 

            asignar_registro(pcb->registros, registro_datos, atoi(leo));
            string_append(&leido, leo);
            int tam_mensaje2 = sizeof(uint16_t)+sizeof(num_frame)+sizeof(int)+sizeof(pid); 
            char* mensaje2= malloc(tam_mensaje2);
            sprintf(mensaje2, "%d/%d/%d/%u", 2,num_frame,0,pid); 

            enviar_a_mem(conexion_memoria_cpu, mensaje2, PED_LECTURA);
   
            int ii = recibir_operacion(conexion_memoria_cpu);
            char* msjj = recibir_pedido(conexion_memoria_cpu);
            char leo2[8];
            int fra2;
            int pag2;
            sscanf(msjj, "%7[^/]/%d", leo2,&fra2,&pag2);
            
            log_info(logger, "PID: <%u> - Accion: <LEER> - Direccion Fisica: <%d> - Valor: <%s>", pid ,fra2, leo2); 
            asignar_registro(pcb->registros, registro_datos, atoi(leo2));
            string_append(&leido, leo2);
            }
           
            free(leido);
            
            break;
        }
        case MOV_OUT:{

            char* registro_datos = list_get(decode->registroCpu,1);
            char* registro_direccion = list_get(decode->registroCpu,0);
            uint8_t dir_logica = (uint8_t) obtener_valor_registro(pcb->registros, registro_direccion);

            log_info(logger, "PID: <%u> - Ejecutando: <MOV_OUT> - <%s %s>", pcb->pid,  registro_direccion, registro_datos);         

            uint8_t valor = (uint8_t) obtener_valor_registro(pcb->registros, registro_datos);
            int tamanio;
    
            if(obtener_tipo_registro(registro_datos) == 8){
                tamanio =1;
            }
            else{
                tamanio = 4;
            }

            char* aescribir = malloc(sizeof(char*));
            aescribir= int_to_char(valor);
            int cant_pags =1;
            bool salir=false;
            char* frame_siguiente;
            t_dir_fisica* dir_fisica = mmu(dir_logica, pcb->pid);

            int num_frame = dir_fisica->nro_frame;
            int desplazamiento = dir_fisica->desplazamiento;
            int tam_primera=tam_pag - desplazamiento;
           
            if(tam_primera<tamanio){
                cant_pags++;
            }
            if(cant_pags ==1){    
                if(num_frame==-1){
                    salir=true;
                }
                if(salir==false){
                int tam_mensaje = sizeof(tamanio)+sizeof(aescribir)+sizeof(num_frame)+sizeof(desplazamiento)+sizeof(pcb->pid); 
                char* mensaje = malloc(tam_mensaje);
                sprintf(mensaje, "%d/%s/%d/%d/%u", tamanio,aescribir,num_frame,desplazamiento,pcb->pid);  

                log_info(logger, "PID: <%u> - Accion: <ESCRIBIR> - Direccion Fisica: <%d> - Valor: <%s>", pcb->pid ,num_frame+desplazamiento, aescribir); 
   
                enviar_a_mem(conexion_memoria_cpu, mensaje,PED_ESCRITURA);

                int i = recibir_operacion(conexion_memoria_cpu);
                int pag;
                int frame;
                char* buffer = recibir_pedido(conexion_memoria_cpu);
                sscanf(buffer, "%d/%d", &frame,&pag);
                }
            }else
            {
                if(num_frame==-1){
                    salir=true;
                }
                if(salir==false){
                    tamanio=2;
                    char* es=decstring(aescribir,0,0);
                    int tam_mensaje1 = sizeof(tamanio)+strlen(es)+sizeof(num_frame)+sizeof(desplazamiento)+sizeof(pcb->pid); 
                    char* mensaje1 = malloc(tam_mensaje1);
                    sprintf(mensaje1, "%d/%s/%d/%d/%u", tamanio,es,num_frame,desplazamiento,pcb->pid);     
                    
                    log_info(logger, "PID: <%u> - Accion: <ESCRIBIR> - Direccion Fisica: <%d> - Valor: <%s>", pcb->pid ,num_frame+desplazamiento, es); 

                    enviar_a_mem(conexion_memoria_cpu, mensaje1,PED_ESCRITURA);

                    int ii = recibir_operacion(conexion_memoria_cpu);
                    int pag1;
                    int frame_sig;
                    char* buffer = recibir_pedido(conexion_memoria_cpu);
                    sscanf(buffer, "%d/%d", &frame_sig,&pag1);
                    int dir_log = pag1*tam_pag;
                    t_dir_fisica* dir_fisica = mmu(dir_log, pcb->pid);
                    int num_frame2 = dir_fisica->nro_frame;             
                    char* es2=decstring(aescribir,1,1);
                    int tam_mensaje2 = sizeof(tamanio)+strlen(es2)+sizeof(num_frame2)+sizeof(pcb->pid)+sizeof(int); 
                    char* mensaje2 = malloc(tam_mensaje2);
                    sprintf(mensaje2, "%d/%s/%d/%d/%u", tamanio,es2,num_frame2,0,pcb->pid);     
                    
                    log_info(logger, "PID: <%u> - Accion: <ESCRIBIR> - Direccion Fisica: <%d> - Valor: <%s>", pcb->pid ,num_frame2, es2);
                    
                    enviar_a_mem(conexion_memoria_cpu, mensaje2,PED_ESCRITURA);
                    int i = recibir_operacion(conexion_memoria_cpu);
                    int pag2;
                    int frame_sig2;
                    char* buffer1 = recibir_pedido(conexion_memoria_cpu);
                    sscanf(buffer1, "%d/%d", &frame_sig2,&pag2);    
                }
            }
            break; 
        }
        case SUM:{
            char* registroDestino = (char*)list_get(decode->registroCpu,0);
            char* registroOrigen = (char*)list_get(decode->registroCpu,1);

            log_info(logger, "PID: <%u> - Ejecutando: <SUM> - <%s %s>", pcb->pid, registroDestino, registroOrigen);        

            uint8_t valor2 = (uint8_t) obtener_valor_registro(pcb->registros, registroDestino);
            uint8_t valor1 = (uint8_t) obtener_valor_registro(pcb->registros, registroOrigen);
            uint8_t suma = valor1 + valor2;
            asignar_registro(pcb->registros, registroDestino, suma);
            break;
        }
        case SUB:{
            char* registroDestino=(char*)list_get(decode->registroCpu, 0);
            char* registroOrigen=(char*)list_get(decode->registroCpu, 1);

            log_info(logger, "PID: <%u> - Ejecutando: <SUB> - <%s %s>", pcb->pid, registroDestino, registroOrigen);        

            uint8_t valor2 = (uint8_t) obtener_valor_registro(pcb->registros, registroDestino);
            uint8_t valor1 = (uint8_t) obtener_valor_registro(pcb->registros, registroOrigen);
            uint8_t resta = valor2 - valor1;
            asignar_registro(pcb->registros, registroDestino, resta);
            break;
        }
        case JNZ:{
            char* registro = (char*)list_get(decode->registroCpu, 0);
            uint32_t instruccion = (uint32_t)list_get(decode->registroCpu, 1);
            
            log_info(cpu_log, "PID: <%u> - Ejecutando: <JNZ> - <%s %u>", pcb->pid, registro, instruccion);
            
            uint8_t valor=obtener_valor_registro(pcb->registros, registro);
            if(valor!=0){
                instrucciones ins= decode->instrucciones;

                pcb->registros->PC= valor;
            }
            break;
        }
        case RESIZE:{
            int tamanio_resize = decode->valor;
            log_info(cpu_log, "PID: <%u> - Ejecutando: <RESIZE> - <%d>", pcb->pid, tamanio_resize);     
            char* mensaje = malloc(sizeof(tamanio_resize)+sizeof(pcb->pid));
            sprintf(mensaje, "%d/%u", tamanio_resize,pcb->pid);       
            enviar_a_mem(conexion_memoria_cpu, mensaje,CPU_RESIZE);
            int a= recibir_operacion(conexion_memoria_cpu);
            char* msj=recibir_mensaje(conexion_memoria_cpu,cpu_log);
            if(strcmp(msj, "outofmem") == 0)
            {
                ret.blockeo = OUT_OF_MEM;
                return ret;
                // enviar_pcb(pcb, kernel_socket);
            }

            break;
        }
        case COPY_STRING:{
            int bytes = decode->valor;
            int cant_char = bytes / sizeof(char); //ver
            uint32_t string_apuntado =obtener_valor_registro(pcb->registros, "SI");
            uint32_t dir_apunt=obtener_valor_registro(pcb->registros, "DI");
            t_dir_fisica* dirstring= mmu(string_apuntado, pcb->pid);
            t_dir_fisica* dir_apuntada = mmu(dir_apunt, pcb->pid);
            int d1=dir_apuntada->nro_frame;
            int d2=dir_apuntada->desplazamiento;
            int d3=dirstring->nro_frame;
            int d4=dirstring->desplazamiento;
            uint32_t pid=pcb->pid;

            char* mensaje = malloc(sizeof(d1)+sizeof(d2)+sizeof(d3)+sizeof(d4)+sizeof(cant_char)+sizeof(pid));
            sprintf(mensaje, "%d/%d/%d/%d/%d/%u", d1,d2,d3,d4,cant_char,pid);      //ver de implementar en demas    
            enviar_a_mem(conexion_memoria_cpu, mensaje,CPY_STRING);

            log_info(logger, "PID: <%u> - Ejecutando: <CPY_STRING> - <%d>", pcb->pid, bytes);  
           
            break;
        }

        case WAIT:{
            ret.nombre_recurso = strdup(decode->recurso); // TODO: Capaz necesita un free

            log_info(cpu_log, "PID: <%u> - Ejecutando: <WAIT> - <%s>", pcb->pid, ret.nombre_recurso);

            ret.blockeo = REC_BLOCK_WAIT;
            return ret; 
        }
        case SIGNAL:{
            ret.nombre_recurso = strdup(decode->recurso); // TODO: Capaz necesita un free

            log_info(cpu_log, "PID: <%u> - Ejecutando: <SIGNAL> - <%s>", pcb->pid, ret.nombre_recurso);

            ret.blockeo = REC_BLOCK_SIGNAL;
            return ret;
        }
        case IO_GEN_SLEEP:{ 
            instruccion_params* parametros =  malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            parametros->params.io_gen_sleep.unidades_trabajo = decode->valor;

            log_info(cpu_log, "PID: <%u> - Ejecutando: <IO_GEN_SLEEP> - <%s %d>", pcb->pid, parametros->interfaz, parametros->params.io_gen_sleep.unidades_trabajo);

            ret.io_opcode = IO_GEN_SLEEP;
            ret.blockeo = IO_BLOCK;

            ret.instrucciones = parametros;

            return ret;
        }
        case IO_STDIN_READ:{

            instruccion_params* parametros =  malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            char* registro_direccion = list_get(decode->registroCpu, 0);
            char* registro_tamanio = list_get(decode->registroCpu, 1);

            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_STDIN_READ> -  <%s %s %s>", pcb->pid, parametros->interfaz,registro_direccion, registro_tamanio);

            int dir_logica=(int)obtener_valor_registro(pcb->registros, registro_direccion);
            parametros->registro_tamanio = (uint32_t)obtener_valor_registro(pcb->registros, registro_tamanio); 
            t_dir_fisica* dir_fisica =  mmu(dir_logica, pcb->pid);
            int tamanio_primera_pagina = tam_pag - dir_fisica->desplazamiento;
            if(tamanio_primera_pagina >= parametros->registro_tamanio){
                parametros->registro_direccion =  malloc(sizeof(t_dir_fisica));
                *parametros->registro_direccion = *dir_fisica;
                parametros->cant_direcciones = 1;
            }
            else{
                int paginas_requeridas = (parametros->registro_tamanio + tam_pag - 1) / tam_pag;
                parametros->registro_direccion = malloc(paginas_requeridas * sizeof(t_dir_fisica));
                parametros->cant_direcciones = paginas_requeridas;
                parametros->registro_direccion[0] = *dir_fisica;
                for (int i = 1; i < paginas_requeridas; i++) {
                    int pagina_actual = (dir_logica / tam_pag) + i;
                    t_dir_fisica* dir_fisica_pagina = mmu(pagina_actual * tam_pag, pcb->pid);
                    parametros->registro_direccion[i] = *dir_fisica_pagina;
                    free(dir_fisica_pagina);
                }
            }
            free(dir_fisica); 
            ret.io_opcode = IO_STDIN_READ;
            ret.blockeo = IO_BLOCK;
            ret.instrucciones = parametros;
            return ret;
        }
        case IO_STDOUT_WRITE:{

            instruccion_params* parametros =  malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            char* registro_direccion = (char*)list_get(decode->registroCpu, 0);
            char* registro_tamanio = (char*)list_get(decode->registroCpu, 1);

            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_STDOUT_WRITE> - <%s %s %s>", pcb->pid, parametros->interfaz, registro_direccion, registro_tamanio);

            int dir_logica=(int)obtener_valor_registro(pcb->registros, registro_direccion);
            parametros->registro_tamanio = (uint32_t)obtener_valor_registro(pcb->registros, registro_tamanio); 
            t_dir_fisica* dir_fisica =  mmu(dir_logica, pcb->pid);
            int tamanio_primera_pagina = tam_pag - dir_fisica->desplazamiento;
            if(tamanio_primera_pagina >= parametros->registro_tamanio){
                parametros->registro_direccion =  malloc(sizeof(t_dir_fisica));
                *parametros->registro_direccion = *dir_fisica;
                parametros->cant_direcciones = 1;
            }
            else{
                int paginas_requeridas = (parametros->registro_tamanio + tam_pag - 1) / tam_pag;
                parametros->registro_direccion = malloc(paginas_requeridas * sizeof(t_dir_fisica));
                parametros->cant_direcciones = paginas_requeridas;
                parametros->registro_direccion[0] = *dir_fisica;
                for (int i = 1; i < paginas_requeridas; i++) {
                    int pagina_actual = (dir_logica / tam_pag) + i;
                    t_dir_fisica* dir_fisica_pagina = mmu(pagina_actual * tam_pag, pcb->pid);
                    parametros->registro_direccion[i] = *dir_fisica_pagina;
                    free(dir_fisica_pagina);
                }
            }
            free(dir_fisica);
            ret.io_opcode = IO_STDOUT_WRITE;
            ret.blockeo = IO_BLOCK;
            ret.instrucciones = parametros;
            return ret;
        }
        case IO_FS_CREATE:{ 

            instruccion_params* parametros = malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            parametros->params.io_fs.nombre_archivo = strdup(decode->archivo);

            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_FS_CREATE> - <%s %s>", pcb->pid, parametros->interfaz, parametros->params.io_fs.nombre_archivo);
    
            ret.blockeo = IO_BLOCK;
            ret.instrucciones = parametros;
            ret.io_opcode = IO_FS_CREATE;
            return ret;
        }

        case IO_FS_DELETE:{ 

            instruccion_params* parametros = malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            parametros->params.io_fs.nombre_archivo = strdup(decode->archivo);
            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_FS_DELETE> - <%s %s>", pcb->pid, parametros->interfaz, parametros->params.io_fs.nombre_archivo);

            ret.blockeo = IO_BLOCK;
            ret.io_opcode = IO_FS_DELETE;
            ret.instrucciones = parametros;
            return ret;
        }
  
        case IO_FS_TRUNCATE:{
       
            instruccion_params* parametros = malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            parametros->params.io_fs.nombre_archivo = strdup(decode->archivo);
            char* registro_tamanio = list_get(decode->registroCpu, 0);
            parametros->registro_tamanio = (uint32_t)obtener_valor_registro(pcb->registros, registro_tamanio);
            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_FS_TRUNCATE> - <%s %s %i>", pcb->pid, parametros->interfaz, parametros->params.io_fs.nombre_archivo, parametros->registro_tamanio);
            ret.blockeo = IO_BLOCK;
            ret.io_opcode = IO_FS_TRUNCATE;
            ret.instrucciones = parametros;
            return ret;
        }
       
        case IO_FS_WRITE:{ 
            
            instruccion_params* parametros = malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            parametros->params.io_fs.nombre_archivo = strdup(decode->archivo);
            char* registro_direccion = list_get(decode->registroCpu, 0);
            char* registro_tamanio = list_get(decode->registroCpu, 1);
            char* registro_puntero_archivo = list_get(decode->registroCpu, 2);
            int dir_logica=(int)obtener_valor_registro(pcb->registros, registro_direccion);
            parametros->registro_tamanio = (uint32_t)obtener_valor_registro(pcb->registros, registro_tamanio); 
            t_dir_fisica* dir_fisica =  mmu(dir_logica, pcb->pid);
            int tamanio_primera_pagina = tam_pag - dir_fisica->desplazamiento;
            if(tamanio_primera_pagina >= parametros->registro_tamanio){
                parametros->registro_direccion =  malloc(sizeof(t_dir_fisica));
                *parametros->registro_direccion = *dir_fisica;
                parametros->cant_direcciones = 1;
            }
            else{
                int paginas_requeridas = (parametros->registro_tamanio + tam_pag - 1) / tam_pag;
                parametros->registro_direccion = malloc(paginas_requeridas * sizeof(t_dir_fisica));
                parametros->cant_direcciones = paginas_requeridas;
                parametros->registro_direccion[0] = *dir_fisica;
                for (int i = 1; i < paginas_requeridas; i++) {
                    int pagina_actual = (dir_logica / tam_pag) + i;
                    t_dir_fisica* dir_fisica_pagina = mmu(pagina_actual * tam_pag, pcb->pid);
                    parametros->registro_direccion[i] = *dir_fisica_pagina;
                    free(dir_fisica_pagina);
                }
            }
            free(dir_fisica);
            parametros->params.io_fs.registro_puntero_archivo = (off_t)obtener_valor_registro(pcb->registros, registro_puntero_archivo);

            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_FS_WRITE> - <%s %s %s %jd>", pcb->pid, parametros->interfaz, registro_tamanio, registro_direccion, parametros->params.io_fs.registro_puntero_archivo);
    
            ret.blockeo = IO_BLOCK;
            ret.io_opcode = IO_FS_WRITE;
            ret.instrucciones = parametros;
            return ret;
        }
        case IO_FS_READ:{ 
           
            instruccion_params* parametros = malloc(sizeof(instruccion_params));
            parametros->interfaz = strdup(decode->interfaz);
            parametros->params.io_fs.nombre_archivo = strdup(decode->archivo);
            char* registro_direccion = list_get(decode->registroCpu, 0);
            char* registro_tamanio = list_get(decode->registroCpu, 1);
            char* registro_puntero_archivo = list_get(decode->registroCpu, 2);
            int dir_logica=(int)obtener_valor_registro(pcb->registros, registro_direccion);
            parametros->registro_tamanio = (uint32_t)obtener_valor_registro(pcb->registros, registro_tamanio); 
            t_dir_fisica* dir_fisica =  mmu(dir_logica, pcb->pid);
            int tamanio_primera_pagina = tam_pag - dir_fisica->desplazamiento;
            if(tamanio_primera_pagina >= parametros->registro_tamanio){
                parametros->registro_direccion =  malloc(sizeof(t_dir_fisica));
                *parametros->registro_direccion = *dir_fisica;
                parametros->cant_direcciones = 1;
            }
            else{
                int paginas_requeridas = (parametros->registro_tamanio + tam_pag - 1) / tam_pag;
                parametros->registro_direccion = malloc(paginas_requeridas * sizeof(t_dir_fisica));
                parametros->cant_direcciones = paginas_requeridas;
                parametros->registro_direccion[0] = *dir_fisica;
                for (int i = 1; i < paginas_requeridas; i++) {
                    int pagina_actual = (dir_logica / tam_pag) + i;
                    t_dir_fisica* dir_fisica_pagina = mmu(pagina_actual * tam_pag, pcb->pid);
                    parametros->registro_direccion[i] = *dir_fisica_pagina;
                    free(dir_fisica_pagina);
                }
            }
            free(dir_fisica);
            parametros->params.io_fs.registro_puntero_archivo = (off_t)obtener_valor_registro(pcb->registros, registro_puntero_archivo);
            log_info(cpu_log, "PID: <%u> - Ejecutando <IO_FS_READ> - <%s %s %s %jd>", pcb->pid, parametros->interfaz, registro_tamanio, registro_direccion, parametros->params.io_fs.registro_puntero_archivo);

            ret.io_opcode = IO_FS_READ;
            ret.blockeo = IO_BLOCK;
            ret.instrucciones = parametros;
            return ret;
        }
        case EXIIT:{
            log_info(logger, "PID: <%u> - Ejecutando: <EXIT>", pcb->pid);
            ret.blockeo = EXIT_BLOCK;
            return ret;
        }
    }

    free(decode);
    return ret;
}

void realizar_ciclo_inst(int conexion, t_pcb* pcb, t_log* logger, int socket_cliente, pthread_mutex_t lock_interrupt){
   
   t_cpu_blockeo blockeo;
   blockeo.blockeo = NO_BLOCK;
   pthread_mutex_unlock(&lock_interrupt);
    while(blockeo.blockeo == NO_BLOCK && !hay_interrupcion)
    {
        log_info(logger, "PID: %d - FETCH - Program counter: <%d>", pcb->pid, pcb->registros->PC);
        char* ins = fetch(conexion,pcb);
        
        t_decode* decodeado = decode(ins);
    
        blockeo = execute(decodeado,pcb, logger);
        
        loggear_registros(pcb, logger);
    }

    pthread_mutex_lock(&lock_interrupt);
    if(hay_interrupcion == 1 && blockeo.blockeo == NO_BLOCK) // Fin de quantum
    {
        hay_interrupcion = 0;
        pthread_mutex_unlock(&lock_interrupt);
        pcb->motivo_desalojo = 1;
        log_debug(log_aux_cpu, "Envio PCB interrumpido por fin de quantum");
        enviar_pcb(pcb, socket_cliente);
        return;
    }
    if(blockeo.blockeo == EXIT_BLOCK)
    {
        hay_interrupcion = 0;
        pthread_mutex_unlock(&lock_interrupt);
        pcb->motivo_desalojo = 0;
        log_debug(log_aux_cpu, "Envio PCB terminado.");
        enviar_pcb(pcb, socket_cliente);
        return;
    }
    if(blockeo.blockeo == IO_BLOCK)
    {
        hay_interrupcion = 0;
        pthread_mutex_unlock(&lock_interrupt);
        pcb->motivo_desalojo = 2;
        log_debug(log_aux_cpu, "Envio PCB desalojado por solicitud a interfaz");
        enviar_pcb(pcb, socket_cliente);

        t_paquete_instruccion* paquete = malloc(sizeof(t_paquete_instruccion));
        paquete->codigo_operacion = blockeo.io_opcode;
        log_debug(log_aux_cpu, "OPCODE enviado: %d", paquete->codigo_operacion);
        enviar_instruccion_a_Kernel(paquete, blockeo.instrucciones, socket_cliente);
        log_debug(log_aux_cpu, "Instruccion enviada al socket: %d.", socket_cliente);
        free(blockeo.instrucciones->interfaz);
        free(blockeo.instrucciones);
        free(paquete);
        return;
    }
    if(blockeo.blockeo == REC_BLOCK_WAIT)
    {
        hay_interrupcion = 0;
        pthread_mutex_unlock(&lock_interrupt);
        pcb->motivo_desalojo = 3;
        log_debug(log_aux_cpu, "Envio PCB desalojado por WAIT");
        enviar_pcb(pcb, socket_cliente);

        log_debug(log_aux_cpu, "Envio el nombre del recurso afectado");
        enviar_mensaje(blockeo.nombre_recurso, socket_cliente);
        // enviar_nombre_recurso(blockeo.nombre_recurso, socket_cliente);
    }
    if(blockeo.blockeo == REC_BLOCK_SIGNAL)
    {
        hay_interrupcion = 0;
        pthread_mutex_unlock(&lock_interrupt);
        pcb->motivo_desalojo = 4;
        log_debug(log_aux_cpu, "Envio PCB desalojado por SIGNAL");
        enviar_pcb(pcb, socket_cliente);

        log_debug(log_aux_cpu, "Envio el nombre del recurso afectado");
        enviar_mensaje(blockeo.nombre_recurso, socket_cliente);
    }
    if(blockeo.blockeo == OUT_OF_MEM)
    {
        hay_interrupcion = 0;
        pthread_mutex_unlock(&lock_interrupt);
        pcb->motivo_desalojo = 5;
        log_debug(log_aux_cpu, "PCB desalojado por out of mem");
        enviar_pcb(pcb, socket_cliente);
    }
}

void loggear_registros(t_pcb* pcb, t_log* logger)
{
    log_debug(log_aux_cpu, "Estado del proceso PID <%d>:", pcb->pid);
    log_debug(log_aux_cpu, "AX: %d", pcb->registros->AX);
    log_debug(log_aux_cpu, "BX: %d", pcb->registros->BX);
    log_debug(log_aux_cpu, "CX: %d", pcb->registros->CX);
    log_debug(log_aux_cpu, "DX: %d", pcb->registros->DX);
    log_debug(log_aux_cpu, "EAX: %d", pcb->registros->EAX);
    log_debug(log_aux_cpu, "EBX: %d", pcb->registros->EBX);
    log_debug(log_aux_cpu, "ECX: %d", pcb->registros->ECX);
    log_debug(log_aux_cpu, "EDX: %d", pcb->registros->EDX);
    log_debug(log_aux_cpu, "SI: %d", pcb->registros->SI);
    log_debug(log_aux_cpu, "DI: %d", pcb->registros->DI);
    log_debug(log_aux_cpu, "PC: %d", pcb->registros->PC);
}
