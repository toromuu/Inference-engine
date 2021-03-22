#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <list>


using namespace std;

//Estructura que define un Atributo
typedef struct Atributo{

    string nombre;
    string tipo; //Nom o NU
    string valor;

}Atributo;

//Estructura que define un Literal
typedef struct Literal{

    //string nombreAtributo;
    struct Atributo atributo;
    string operador;
    string valor;
    bool resuelto;

}Literal;

//Estructura que define una regla
typedef struct Regla {

    string identificadorR; //va desde 1 hasta N
    int peso;
    list<Literal> LHS; //Parte Izquierda se corresponde con el literal A
    struct Literal RHS;//Parte Derecha se corresponde con el literal B
    bool aplicada; //Se utiliza para gestionar las ReglasAplicadas con facilidad en vez de tener dos conjuntos

}Regla;


/* Función para leer el contenido del fichero de configuración
y parsearlo a las estructuras correspondientes.
El recorrido por el mismo se realizara palabra a palabra, obviando
espacios en blanco y saltos de línea.
*/
void parseFicheroConf( char *rutaFicheroConf, list<Atributo> *listAtributos, Atributo *atribObjetivo, list<Regla> *listReglas ){

    string auxPalabra; //Almacena la palabra actual
    int n; //Almacena el numero de Atributos
    ifstream fconf(rutaFicheroConf);

    //En primer lugar se parsearan los Atributos
    fconf>>auxPalabra; //La primera linea se descarta
    fconf>>n; //La segunda linea contiene el numero de atributos a tratar

    //Las n siguientes lineas se corresponden con un atributo
    for( int i = 0; i < n; i++){
        Atributo *atrib = new Atributo();
        //Se almacena el nombre y el tipo
        fconf>>atrib->nombre;
        fconf>>atrib->tipo;
        if (atrib->tipo == "Nom"){ //Si es de tipo Nom hay que leer sus valores
            fconf>>auxPalabra;
        }
        listAtributos->push_back(*atrib);//Se inserta el nuevo atributo en la lista
    }

    //A continuación hay que leer el atributo Objetivo
    fconf>>auxPalabra; //Se salta la linea "OBJETIVO"
    fconf>>atribObjetivo->nombre;

     //De la misma forma para los pesos de las reglas
    fconf>>auxPalabra; //Se salta la linea "PRIORIDADES-REGLAS"
    fconf>>n; //Numero de reglas
    for(int i = 0; i < n; i++){
        Regla *regl = new Regla();
        fconf>>regl->peso;
        regl->aplicada = false; //Se inicializan como no aplicadas
        listReglas->push_back(*regl);
    }
    fconf.close();

}

/* Función para leer el contenido del fichero de la BC*/
void parseFicheroBC( char *rutaFicheroBC, list<Atributo> *listAtributos, list<Regla> *listReglas,char*nombreDominio){

    ifstream fBC(rutaFicheroBC);
    string auxPalabra; //Almacena la palabra actual
    int n;
    fBC.getline(nombreDominio, 1024); //Obtener toda la primera linea
    fBC>>n; //Numero de reglas de la BC

    //Se itera la lista de reglas
    for( list<Regla>::iterator it = listReglas->begin(); it != listReglas->end(); it++){

        //Se obtiene el identificador de cada regla , ejm R1
        fBC>>it->identificadorR;

        fBC>>auxPalabra;
        // Se recorre toda la linea hasta el "Entonces", de esta forma se puede leer toda la parte izquierda de la regla
        while( auxPalabra != "Entonces") {
            //Hay que construir el literal A, este puede ser una conjuncion de literales
            //Obtenemos el atributo
            Atributo *atribLitA = new Atributo();
            fBC>>atribLitA->nombre;

            //Para obtener el tipo del atributo hay que hacer una busqueda por el mismo nombre en la lista de atributos
            bool breaker = false;
            list<Atributo>::iterator itatribAux = listAtributos->begin();
            while ((itatribAux != listAtributos->end()) && (breaker == false)){
                Atributo atribAux = *itatribAux;
                if ( atribLitA->nombre ==atribAux.nombre){
                    atribLitA->tipo = atribAux.tipo;
                    breaker = true; //Con encontrar la primera coincidencia sobra, no hace falta iterarlo todo
                }
                itatribAux++;
            }

            //Se crea el literal
            Literal *literal = new Literal();
            literal->atributo = *atribLitA; //Se le asigna el atributo
            fBC>>literal->operador; //El operador
            fBC>>literal->valor; //El valor
            literal->resuelto=false;
            it->LHS.push_back(*literal); //Se añade el literal a la parte izquierda

        fBC>>auxPalabra; //Se avanza en la linea
        }
        //El resto de la linea corresponde con la parte derecha de la regla

        //Hay que construir el literal B
        Atributo *atribLitB = new Atributo();
        Literal *literalB = new Literal();
        fBC>>atribLitB->nombre;
        literalB->atributo=*atribLitB;
        fBC>>literalB->operador;
        fBC>>literalB->valor;
        it->RHS = *literalB;

    }
    fBC.close(); //Se cierra el fichero
}

/*Función para leer el contenido del fichero de la BH
Cada linea del fichero contiene un literal que sera almacenado en la lista de hechos
*/
void parseFicheroBH( char *rutaFicheroBH, list<Literal> *listHechos){

    ifstream fBH(rutaFicheroBH);
    int n; //Almacena el numero de hechos
    fBH>>n;

    for(int i=0; i<n; i++){ //Se recorren los hechos
        Literal *literal = new Literal();
        Atributo *atrib = new Atributo();
        fBH>>atrib->nombre; //Se obtiene el nombre del atributo
        literal->atributo = *atrib;
        fBH>>literal->operador;
        fBH>>literal->valor;
        listHechos->push_back(*literal); //Se inserta en la lista de hechos
    }

    fBH.close();

}


/*
En esta función se determina que regla se va a expandir en función de su peso
y se elimina del conjunto vacio
*/
Regla resolver(list<Regla> *conjuntoConflicto){

    int pesoReglaCandidata = -1; //Maximo inicial
    int iIteradorRegla = 0;  //Se usaran para almacenar la posicion del iterador, con ello podremos eliminar de una lista
    int iIteradorActual = 0;

    list<Regla>::iterator it;
    // Se itera el conjunto conflicto, buscando la regla con el peso mas elevado
    for( it = conjuntoConflicto->begin(); it != conjuntoConflicto->end(); it++){

        Regla R = *it;
        if (R.peso > pesoReglaCandidata){
            pesoReglaCandidata = R.peso;
            iIteradorRegla = iIteradorActual;
        }
        iIteradorActual++;
    }

    /* Se itera de nuevo el conjunto conflicto para eliminar la regla con el mayor peso
    Hay que tener en cuenta que la eliminacion  no se puede hacer hasta hacer un recorrido completo ya que
    hasta entonces no se sabrá que regla es.
    Para eliminar un elemento de la lista en necesario pasar como parametro el iterador de la posicion del elemento
    que se desea eliminar.
    */

    iIteradorActual=0;
    it = conjuntoConflicto->begin();
    while((iIteradorRegla != iIteradorActual) && (it != conjuntoConflicto->end())){
        it++;
        iIteradorActual++;

    }
    //El bucle se romper con el iterador apuntando a la regla con el peso más elevado
    Regla regla = *it;
    conjuntoConflicto->erase(it); //Se elimina la regla del conjunto
    return regla;
}


/*
En esta función se añade a la base de hechos el literal B derivado de una regla
Para ello hay que recorrer la Base de Hechos y ver si se encuentra ya incluido
*/
void aplicar(list<Literal> *listHechos , Regla R){


    bool incluido = false;
    for(list<Literal>::iterator it = listHechos->begin(); it != listHechos->end(); it++){
        Literal literal = *it;
        if (literal.atributo.nombre == R.RHS.atributo.nombre){
            incluido = true;
        }
    }
    if(incluido == false){
        listHechos->push_back(R.RHS);
    }

}


// En esta función se comprueba si el atributo objetivo se encuentra en la base de hechos
bool contenida(Atributo *atribObj, list<Literal> *listHechos){

    for( list<Literal>::iterator it = listHechos->begin(); it != listHechos->end() ; it++){
        Literal literal = *it;
        if (literal.atributo.nombre == atribObj->nombre){
            atribObj->valor = literal.valor;
            return true;
        }
    }
    return false;

}


/*
 En esta función se eliminan la regla aplicada del conjunto conflicto
 Para ello se establece su valor en la propiedad aplicada a verdadero
 */
void reglasAplicadas(list<Regla>*listReglas, string identificadorR){ //ReglasAplicadas=ReglasAplicadas+{R}

    bool condicionParada = false;
    list<Regla>::iterator it = listReglas->begin();
    while((it != listReglas->end()) && (condicionParada == false)){
        Regla R = *it;
        if( R.identificadorR == identificadorR ){ //Una vez establecida como aplicada no hace falta serguir iterando
            it->aplicada = true;
            condicionParada = true;
        }
        it++;
    }

}


// Función que comprueba si una regla se encuentra en el conjunto conflicto
bool comprobarConflicto(Regla r ,list<Regla> *conjuntConf){

    list<Regla>::iterator it;
    for(it = conjuntConf->begin(); it != conjuntConf->end(); it++){
        Regla r2 = *it;
        if (r2.identificadorR == r.identificadorR)
            return true;
    }
    return false;

}


/* Esta funcion añade al conjunto conflicto las reglas
con literales que cumplen con algun hecho de la base de hechos

*/
void equiparar( list<Regla> *conjuntoConflicto, list<Regla> *listReglas, list<Literal> *listHechos, ofstream *salida1BHXX){
    //Se iteran todas las reglas
    for( list<Regla>::iterator it = listReglas->begin(); it != listReglas->end(); it++){
        Regla R = *it;

        if( R.aplicada == false ){ //Si no ha sido aplicada

            list<Regla>::iterator itconflict = conjuntoConflicto->begin();
            if( comprobarConflicto(R,conjuntoConflicto) ==false ){ //Se comprueba si ya esta incluida en el conjunto conflicto

                    // En caso de no estar incluida, se comprueba si la regla se cumple en base a los hechos actuales
                    // Se iteran los literales de la parte izquierda
                    for(list<Literal>::iterator itLHS = R.LHS.begin(); itLHS != R.LHS.end(); itLHS++){
                        //Se iteran los literales de la base de Hechos
                        for(list<Literal>::iterator itHechos = listHechos->begin(); itHechos != listHechos->end(); itHechos++){

                            Literal literal1 = *itLHS;
                            Literal literal2 = *itHechos;

                            if (literal1.atributo.nombre == literal2.atributo.nombre){ //Si el atributo del literal es el mismo
                                //Dependiendo del tipo de atributo tendra un tratamiento u otro

                                //Si es del tipo numerico, se hace la comprobacion segun el operador
                                if (literal1.atributo.tipo == "NU"){

                                    int lit1 = atoi(literal1.valor.c_str()); //Hay que convertir los valores a tipo numerico para comparar
                                    int lit2 = atoi(literal2.valor.c_str());


                                    if(( literal1.operador == "=")&&(lit2 == lit1)){
                                        itLHS->resuelto = true;
                                    }
                                    else if(( literal1.operador == ">=") && (lit2 >= lit1) ){
                                        itLHS->resuelto = true;
                                    }
                                    else if(( literal1.operador == ">") && (lit2 > lit1) ){
                                        itLHS->resuelto = true;
                                    }
                                    else if( (literal1.operador == "<")&&(lit2 < lit1)){
                                        itLHS->resuelto = true;
                                    }
                                    else if( (literal1.operador == "<=") && (lit2 <= lit1) ){
                                        itLHS->resuelto = true;
                                    }

                                }else{
                                    // Si es de tipo nominal solo se comprueba su valor
                                    if (literal1.valor == literal2.valor){
                                        itLHS->resuelto = true;
                                    }
                                } //Comprobacion nominal

                            } //Comprobacion atributo

                        }

                    }//Iteracion de literales de la regla

                /* Se vuelven a iterar  los literales de la parte izquierda con el fin de comprobar si la regla se cumple
                en base a los hechos, porque todos sus literales han sido resueltos
                */
                bool literalres = true;
                for(list<Literal>::iterator itLHS = R.LHS.begin(); itLHS != R.LHS.end(); itLHS++){
                    Literal literal = *itLHS;
                    if (literal.resuelto == false){
                        literalres = false;
                    }
                }

                 if (literalres == true){ // Si todos los literales han sido resueltos, entonces se verifica que la regla se cumple y por tanto se añade

                        *salida1BHXX << "Se ha insertado la regla con identificador " << R.identificadorR << " al Conjunto Conflicto" << endl;
                        conjuntoConflicto->push_back(R);
                }


            } //condicion pertenencia conjunto conflicto


        }//condicion de regla aplicada


    }//Bucle externo
}


/* En esta función se realizará el algoritmo de ENCADENAMIENTO-HACIA-DELANTE
descrito en teoria

funcion ENCADENAMIENTO-HACIA-DELANTE
    BH=HechosIniciales; ConjuntoConflicto={ }; ReglasAplicadas={ };

    repetir
        ConjuntoConflicto=ConjuntoConflicto-ReglasAplicadas;
        ConjuntoConflicto=ConjuntoConflicto+Equiparar(antecedente(BC),BH);
    si NoVacio(ConjuntoConflicto) entonces
        R=Resolver(ConjuntoConflicto);
        NuevosHechos=Aplicar(R,BH); ReglasAplicadas=ReglasAplicadas+{R}
        Actualizar(BH,NuevosHechos);
    fin si
    hasta Contenida(Meta,BH) o Vacio(ConjuntoConflicto);
    si Contenida(Meta,BH) entonces devolver ‘’exito’’
fin si


Adicionalmente, como tiene toda la información necesaria, se aprovechara
para generar los ficheros de salida

    Salida1-XXX.txt
    tiene que cumplir con la siguiente especificación:
        el dominio de aplicación,
        el atributo objetivo, la BH inicial y los elementos
        necesarios para indicar el proceso de razonamiento seguido por el SBR.

    Salida2-XXX.txt
        conteniendo el dominio de aplicación,
        el atributo objetivo junto a su valor y
        la solución obtenida (secuencia de reglas que componen la solución).

*/


void motorInferencia(char*rutaFicheroBH,  Atributo*atribObjetivo,char*nombreDominio,list<Regla>*listReglas,list<Literal>*listHechos){


    list<Regla> listSolucion;
    char nombrefichSalida[80] = { 0 };
    strcat(nombrefichSalida,"Salida1-");
    strcat(nombrefichSalida,rutaFicheroBH);//Se concatena salida1 con el nombre del ficheroH
    ofstream salida1BHXX(nombrefichSalida);

    salida1BHXX << nombreDominio << endl << endl;
    salida1BHXX << "**********************************"<<endl;
    salida1BHXX << "**OBJETIVO: " << atribObjetivo->nombre <<endl;
    salida1BHXX << "**********************************" <<endl;
    salida1BHXX << "**BASE DE HECHOS INICIAL: " << endl;
    for(list<Literal>::iterator it = listHechos->begin(); it != listHechos->end(); it++){
        Literal literal = *it;
        salida1BHXX << literal.atributo.nombre << " " << literal.operador << " " << literal.valor << endl;
    }
    salida1BHXX << "**********************************"<<endl;


    salida1BHXX << "**SE VA A REALIZAR ENCADENAMIENTO-HACIA-DELANTE**" << endl << endl;
    salida1BHXX << "Se parten de los Hechos Iniciales descritos" << endl; //BH=HechosIniciales;
    salida1BHXX << "Se crea el Conjunto conflicto vacio" << endl << endl;
    list<Regla> conjuntoConflicto; //ConjuntoConflicto={ }
    int contadorIteracion =1;
    do {
        salida1BHXX << "##### ITERACION " << contadorIteracion << "#####"<< endl;
        //ConjuntoConflicto=ConjuntoConflicto-ReglasAplicadas; ->No es necesario ya que se actualizara con ReglasAplicadas
        salida1BHXX << "Se lleva a cabo la Equiparacion" << endl;
        equiparar(&conjuntoConflicto,listReglas,listHechos,&salida1BHXX);
        //ConjuntoConflicto=ConjuntoConflicto+Equiparar(antecedente(BC),BH);
        salida1BHXX << "El Conjunto Conflicto contiene " << conjuntoConflicto.size() << " Reglas" << endl;
        if (conjuntoConflicto.size()>0){
            Regla R = resolver(&conjuntoConflicto); //R=Resolver(ConjuntoConflicto);
            salida1BHXX << "Se ha seleccionado la regla con identificador  " << R.identificadorR << " con un peso de " << R.peso << endl;
            listSolucion.push_back(R); //Hay que añadir la regla para recomponer la solucion
            reglasAplicadas( listReglas, R.identificadorR); //ReglasAplicadas=ReglasAplicadas+{R}
            aplicar(listHechos,R);//NuevosHechos=Aplicar(R,BH); -> implica Actualizar(BH,NuevosHechos);
            salida1BHXX << "Se aplica la regla " << R.identificadorR << " se añade a la base de Hechos el literal { " << R.RHS.atributo.nombre << " " << R.RHS.operador << " " << R.RHS.valor  << " }" << endl;
            equiparar(&conjuntoConflicto,listReglas,listHechos,&salida1BHXX);
        }
        salida1BHXX << "##########################################" << endl << endl;
        contadorIteracion++;

    } while ((contenida(atribObjetivo, listHechos)==false)&&(conjuntoConflicto.size()>0)); //Contenida(Meta,BH) o Vacio(ConjuntoConflicto);


    salida1BHXX << "--EL PROCEDIMIENTO HA TERMINADO--" << endl << endl;

    if (atribObjetivo->valor.empty()) {
         salida1BHXX << "Con los datos disponibles no se ha podido inferir una solucion" << endl;
    }else {
         salida1BHXX << "Se ha inferido la siguiente solucion: " << atribObjetivo->valor << endl;
    }

    salida1BHXX << "**********************************"<<endl;
    salida1BHXX << "**BASE DE HECHOS FINAL: " << endl << endl;
    for (list<Literal>::iterator itHechos = listHechos ->begin(); itHechos != listHechos->end(); itHechos++){
        Literal literal = *itHechos;
        salida1BHXX << literal.atributo.nombre << " " << literal.operador << " " << literal.valor << endl;
    }
    salida1BHXX << "*****************"<<endl;
    salida1BHXX.close();

    /*
    Salida2-XXX.txt conteniendo el dominio de aplicación,
    el atributo objetivo junto a su valor y
    la solución obtenida (secuencia de reglas que componen la solución).
    */

    char nombrefichSalida2[80] = { 0 };
    strcat(nombrefichSalida2,"Salida2-");
    strcat(nombrefichSalida2,rutaFicheroBH);//Se concatena salida1 con el nombre del ficheroH
    ofstream salida2BHXX(nombrefichSalida2);

    salida2BHXX << nombreDominio << endl << endl;
    salida2BHXX << "**********************************"<<endl;
    salida2BHXX << "OBJETIVO: " << atribObjetivo->nombre << endl;
    salida2BHXX << "VALOR DEL OBJETIVO: " << atribObjetivo->valor << endl;
    salida2BHXX << "PASOS-SOLUCION: " << endl;

    for (list<Regla>::iterator it= listSolucion.begin(); it != listSolucion.end(); it++){
        Regla R = *it;
        salida2BHXX << R.identificadorR << " ";
    }
    salida2BHXX << endl;
    salida2BHXX << "**********************************"<<endl;
    salida2BHXX.close();

}


int main (int argc, char **argv) {

    //Estructuras necesarias
    list<Atributo> listAtributos;
    Atributo *atribObjetivo = new struct Atributo;
    list<Regla> listReglas;
    char nombreDominio[1024];
    list<Literal> listHechos;

    //Lectura de parametros de entrada
    char *rutaFicheroConf = argv[1];
    char *rutaFicheroBC = argv[2];
    char *rutaFicheroBH = argv[3];

    // Tratamiento del fichero de configuracion
    parseFicheroConf( rutaFicheroConf, &listAtributos, atribObjetivo, &listReglas );

    // Tratamiento del fichero con la BC
    parseFicheroBC( rutaFicheroBC,&listAtributos, &listReglas , nombreDominio);

    // Tratamiento del fichero con la BH
    parseFicheroBH( rutaFicheroBH,&listHechos);

    // Se lleva a cabo el algoritmo ENCADENAMIENTO-HACIA-DELANTE y la impresion
    motorInferencia(rutaFicheroBH,atribObjetivo,nombreDominio,&listReglas,&listHechos);

}

