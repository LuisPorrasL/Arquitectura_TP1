#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"


// Constantes:
int ROOT = 0;

int esPrimo(int numero){
    
    int conteoPrimo = 0;
    int i;
    
    for(i=1;i<=numero;i++)
        if(numero%i==0)
            conteoPrimo++;
    return (conteoPrimo==2?1:0);
}

int contarPrimosTotalesMyPorColumnaM(int vectorParteM[], int dimensionesParteM, int dimensionFila, int vectorConteoColumnas[]){
    int indice;
    int posResultado = 0;
    int acumuladorPrimos = 0;
    // Primero se limpia el vector de resultados    
    for (indice = 0; indice < dimensionFila; ++indice) vectorConteoColumnas[indice] = 0;
    // Se recorre y se suman los primos por columna en el vector de resultados y el total de primos
    for (indice = 0; indice < dimensionesParteM; ++ indice){
        if (esPrimo(vectorParteM[indice])){
            ++acumuladorPrimos;
            ++vectorConteoColumnas[posResultado];           
        }
        posResultado = (posResultado + 1)% dimensionFila;
    }
    return acumuladorPrimos;
}

int preguntar_n(int cantidad_procesos){
    int n = 0;
    while(!n){
        printf("\nDigite el tamaño \"n\" de las matrices cuadradas A y B.\nRecuerde que \"n\" debe ser multiplo de %d y mayor que 0.\n", cantidad_procesos);
        scanf("%d", &n);
        if(n%cantidad_procesos != 0 || n == 0){
            printf("\nPor favor digite un valor de \"n\" valido.\n\n");
            n = 0;
        }
    }
    return n;
}

void llenar_vector_aleatoreamente(int vector[], int tamanno, int valor_aleatorio_minimo, int valor_aleatorio_maximo){
    time_t t;
    srand( (unsigned) time (&t));
    for(int indice = 0; indice < tamanno; ++indice)vector[indice] = (int)rand() % (valor_aleatorio_maximo + 1 - valor_aleatorio_minimo) + valor_aleatorio_minimo;
}

void imprimir_matriz_cuadrada_memoria_continua_por_filas(int matriz[], int tamanno, FILE* archivo){
    for(int indice = 0; indice < (tamanno*tamanno); ++indice){
        if(indice%tamanno == 0) fprintf(archivo, "\n");
        fprintf(archivo,"%4d", matriz[indice]);
    }
    fprintf(archivo, "\n\n");
}

void imprimir_filas_matriz_cuadrada_memoria_continua_por_filas(int filas[], int numero_filas, int tamanno, FILE* archivo){
    for(int indice = 0; indice < (numero_filas*tamanno); ++indice){
        if(indice%tamanno == 0) fprintf(archivo, "\n");
        fprintf(archivo,"%4d", filas[indice]);
    }
    fprintf(archivo, "\n");
}

void obtener_fila_matriz_cuadrada_memoria_continual_por_fila(int matriz[], int indice_fila, int tamanno, int resultado[]){
    for(int indice_matriz = indice_fila*tamanno, contador = 0; indice_matriz < (indice_fila+1)*tamanno; ++indice_matriz, ++contador) resultado[contador] = matriz[indice_matriz];
}

void obtener_columna_matriz_cuadrada_memoria_continual_por_fila(int matriz[], int indice_columna, int tamanno, int resultado[]){
    for(int indice_matriz = indice_columna, contador = 0; indice_matriz < (tamanno*tamanno); indice_matriz+=tamanno, ++contador) resultado[contador] = matriz[indice_matriz];
}

int calcular_producto_punto(int vector_a[], int vector_b[], int tamanno){
    int resultado = 0;
    for(int indice = 0; indice < tamanno; ++indice) resultado += vector_a[indice]*vector_b[indice];
    return resultado;
}

void calcular_producto_parcial_matrices_cuadradas_memoria_continua_por_filas(int filas[], int matriz_columnas[], int numero_filas, int tamanno, int resultado_parcial[]){
    int indice_fila, indice_columna, ultimo_indice_fila = -1;
    int *tmp_fila = (int*)malloc(sizeof(int)*tamanno);
    int *tmp_col = (int*)malloc(sizeof(int)*tamanno);
    for(int indice_resultado_parcial = 0; indice_resultado_parcial < (numero_filas*tamanno); ++indice_resultado_parcial){
        indice_fila = indice_resultado_parcial/tamanno;
        indice_columna = indice_resultado_parcial%tamanno;
        if(ultimo_indice_fila != indice_fila){
            obtener_fila_matriz_cuadrada_memoria_continual_por_fila(filas, indice_fila, tamanno, tmp_fila);
            ultimo_indice_fila = indice_fila;
        }
        obtener_columna_matriz_cuadrada_memoria_continual_por_fila(matriz_columnas, indice_columna, tamanno, tmp_col);

        resultado_parcial[indice_resultado_parcial] = calcular_producto_punto(tmp_fila, tmp_col, tamanno);
    }
    free(tmp_fila);
    free(tmp_col);
}

int main(int argc, char* argv[]) {
    int *A, *B, *M, *parte_A, *parte_M;
    int n, cantidad_procesos, proceso_id, tamanno_nombre_procesador;
    char nombre_procesador[MPI_MAX_PROCESSOR_NAME];
    int tp, myTp = 0; //  Acumulador para el conteo de primos en la matriz M
    int* myP, *P; // Vectores para el conteo de los primos por fila
    FILE* archivo = stdout;

    MPI_Init(&argc, &argv); // Inicializacion del ambiente para MPI.
    MPI_Comm_size(MPI_COMM_WORLD, &cantidad_procesos); // Se le pide al comunicador MPI_COMM_WORLD que almacene en cantidad_procesos el numero de procesos de ese comunicador.
    MPI_Comm_rank(MPI_COMM_WORLD, &proceso_id); // Se le pide al comunicador MPI_COMM_WORL que devuelva en la variable proceso_id la identificacion del proceso "llamador".
    
    MPI_Get_processor_name(nombre_procesador,&tamanno_nombre_procesador);
    printf("Proceso %d de %d en %s\n", proceso_id, cantidad_procesos, nombre_procesador); //Cada proceso despliega su identificacion y el nombre de la computadora en la que corre.
    MPI_Barrier(MPI_COMM_WORLD);

    if(proceso_id == ROOT){
        // Se le pide al usuario el tamaño "n" de las matrices cuadradas A y B.
        n = preguntar_n(cantidad_procesos);

        // Se crean (aloja memoria) las matrices A y B con tamaño nxn.
        A = (int*)malloc(sizeof(int)*(n*n));
		B = (int*)malloc(sizeof(int)*(n*n));

        // Se llenan las matrices A y B con valores aleatorios.
        llenar_vector_aleatoreamente(A, (n*n), 0, 5);
        llenar_vector_aleatoreamente(B, (n*n), 0, 2);

        fprintf(archivo, "\nA:\n");
        imprimir_matriz_cuadrada_memoria_continua_por_filas(A, n, archivo);
        fprintf(archivo, "\nB:\n");
        imprimir_matriz_cuadrada_memoria_continua_por_filas(B, n, archivo);
    }

    // Necesito enviar a todos los procesos el valor de n, para que estos puedan reservar la memoria para sus partes de A y B.
    MPI_Bcast(&n, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
    // Necesito repartir las matrices A y B.
    // B ya que es de la que se ocupan las columnas se va a repartir completa entre todos los procesos.
    if(proceso_id != ROOT) B = (int*)malloc(sizeof(int)*(n*n));
    MPI_Bcast(B, (n*n), MPI_INT, ROOT, MPI_COMM_WORLD);
    // A ya que solo se ocupan sus filas, se va a repartir sus filas entre los procesos, para así paralelisar el calculo de M.
    int numero_filas_parte_A = n/cantidad_procesos;
    int tamanno_parte_A = (numero_filas_parte_A)*n;
    parte_A = (int*)malloc(sizeof(int)*tamanno_parte_A);
    MPI_Scatter(A, tamanno_parte_A, MPI_INT, parte_A, tamanno_parte_A, MPI_INT, ROOT, MPI_COMM_WORLD);

    /*printf("Proceso %d de %d en %s\n", proceso_id, cantidad_procesos, nombre_procesador); //Cada proceso despliega su identificacion y el nombre de la computadora en la que corre.
    imprimir_matriz_cuadrada_memoria_continua_por_filas(B, (n), stdout);
    imprimir_filas_matriz_cuadrada_memoria_continua_por_filas(parte_A, numero_filas_parte_A, n, stdout);
    MPI_Barrier(MPI_COMM_WORLD);*/

    // Necesito que todos los procesos calculen su parte de M
    parte_M = (int*)malloc(sizeof(int)*tamanno_parte_A);
    calcular_producto_parcial_matrices_cuadradas_memoria_continua_por_filas(parte_A, B, numero_filas_parte_A, n, parte_M);

    /*printf("Proceso %d de %d en %s\n", proceso_id, cantidad_procesos, nombre_procesador); //Cada proceso despliega su identificacion y el nombre de la computadora en la que corre.
    imprimir_filas_matriz_cuadrada_memoria_continua_por_filas(parte_M, numero_filas_parte_A, n, stdout);
    MPI_Barrier(MPI_COMM_WORLD);*/

    // Necesito armar la matriz M recuperando las partes calculadas por cada proceso
    M = (int*)malloc(sizeof(int)*(n*n));
    MPI_Gather(parte_M, tamanno_parte_A, MPI_INT, M, tamanno_parte_A, MPI_INT, ROOT, MPI_COMM_WORLD);
    if(proceso_id == ROOT){
        fprintf(archivo, "\nM:\n");
        imprimir_matriz_cuadrada_memoria_continua_por_filas(M, n, archivo);
    }

    // Se realiza el conteo total de primos en M por cada proceso y el conteo por Columna
    MPI_Barrier(MPI_COMM_WORLD); // ---> quitar luego
    // Se preparan los vectores
    if(proceso_id == ROOT)  P = (int*)malloc(sizeof(int)*n); // proceso 0 crea P
    myP = (int*)malloc(sizeof(int)*n); // todos los procesos crea su P para acumular
    // Se llama al metodo que hace todo el conteo
    myTp = contarPrimosTotalesMyPorColumnaM(parte_M,tamanno_parte_A,n,myP);    
    //printf("Soy el proceso %d y se que conte %d primos en M\n",proceso_id,myTp);    
    // Se realiza el reduce con operacion de suma de los primos que cada proceso conto hacia el proc ROOT
    MPI_Reduce(&myTp,&tp,1,MPI_INT,MPI_SUM,ROOT,MPI_COMM_WORLD);
    if(proceso_id == ROOT) fprintf(archivo, "\nEl total de primos de la Matriz M es %d\n",tp);
    MPI_Barrier(MPI_COMM_WORLD); // ---> quitar luego
    // Se realiza el reduce de los resultados de las sumas
    MPI_Reduce(myP,P,n,MPI_INT,MPI_SUM,ROOT,MPI_COMM_WORLD);

    // Proceso cero imprime 
    MPI_Barrier(MPI_COMM_WORLD); // ---> quitar luego    
    if (proceso_id == ROOT){
        fprintf(archivo, "\n");
        int y;
        fprintf(archivo, "Los primos por columnas en M son:\n");
        for (y = 0; y < n; ++ y)
            fprintf(archivo, "P[%d] = %d\n", y, P[y]);
    }

    // Se libera memoria.
    free(B);
    free(M);
    free(parte_A);
    free(parte_M);
    free(myP);

    // Solamente el proceso ROOT libera A
    if(proceso_id == ROOT) {free(A); free(P);}

    MPI_Finalize(); // Se termina el ambiente MPI.
    return 0;
}

