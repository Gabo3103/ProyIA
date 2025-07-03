#include <iostream>             
#include <vector>                
#include <fstream>              
#include <cmath>                 
#include <unordered_map>        
#include <unordered_set>        
#include <algorithm>           
#include <random>               
#include <ctime>                
#include <iomanip>              

using namespace std;

//=====================REPRESENTACIÓN=======================
//==========================================================

struct Nodo {
    int id;
    int tipo; 
    float x, y;
    float demanda = 0;
};

struct Ruta {
    vector<int> nodos;
    float carga_utilizada = 0;
    float carga_entregada = 0;
    float carga_recogida = 0;
    float costo_total = 0;
};

struct Solucion {
    vector<Ruta> rutas;
    float costo_total = 0;
};

//=====================FUNCIÓN EVALUACIÓN=======================
//==========================================================

// Función para calcular la distancia euclidiana entre dos nodos
float distancia(const Nodo& a, const Nodo& b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

// Suma las distancias entre pares consecutivos de nodos en una ruta
float calcular_costo_ruta(const Ruta& ruta, const unordered_map<int, Nodo>& mapa_nodos) {
    float costo = 0.0;
    for (size_t i = 1; i < ruta.nodos.size(); ++i) {
        const Nodo& anterior = mapa_nodos.at(ruta.nodos[i - 1]);
        const Nodo& actual = mapa_nodos.at(ruta.nodos[i]);
        costo += distancia(anterior, actual);
    }
    return costo;
}

// Suma todos los costos individuales de las rutas
float calcular_costo_total(Solucion& sol, const unordered_map<int, Nodo>& mapa_nodos) {
    float total = 0.0;
    for (auto& ruta : sol.rutas) {
        ruta.costo_total = calcular_costo_ruta(ruta, mapa_nodos);
        total += ruta.costo_total;
    }
    sol.costo_total = total;
    return total;
}

//=====================LECTURA INSTANCIA=======================
//=============================================================

void leer_instancia(const string& nombre_archivo, vector<Nodo>& nodos, unordered_map<int, Nodo>& mapa_nodos,
                    int& cantidad_vehiculos, float& capacidad_vehiculo, int& id_deposito) {
    ifstream archivo("instancias/" + nombre_archivo);
    if (!archivo.is_open()) {
        cerr << "Error: No se pudo abrir el archivo." << endl;
        exit(1);
    }

    int cantidad_nodos;
    archivo >> cantidad_nodos;
    nodos.resize(cantidad_nodos);

    for (int i = 0; i < cantidad_nodos; ++i) {
        archivo >> nodos[i].tipo >> nodos[i].id >> nodos[i].x >> nodos[i].y;
        if (nodos[i].tipo == 0) id_deposito = nodos[i].id;
    }
    archivo >> cantidad_vehiculos >> capacidad_vehiculo;

    for (int i = 0; i < cantidad_nodos; ++i) {
        int id;
        float demanda;
        archivo >> id >> demanda;
        for (auto& nodo : nodos) {
            if (nodo.id == id) {
                nodo.demanda = demanda;
                break;
            }
        }
    }

    for (const auto& nodo : nodos) {
        mapa_nodos[nodo.id] = nodo;
    }
    archivo.close();
}

//=====================SALIDA DE INSTANCIA=======================
//===============================================================

void escribir_salida(const string& nombre_instancia, const Solucion& sol,
                    const unordered_map<int, Nodo>& mapa_nodos, double tiempo_ejecucion) {
    ofstream archivo("salidas/" + nombre_instancia + ".out");
    if (!archivo.is_open()) {
        cerr << "Error al abrir archivo de salida." << endl;
        return;
    }

    int total_clientes = 0;
    for (const auto& ruta : sol.rutas) {
        for (int id : ruta.nodos) {
            if (mapa_nodos.at(id).tipo == 1 || mapa_nodos.at(id).tipo == 2)
                total_clientes++;
        }
    }

    archivo << fixed << setprecision(2)
            << sol.costo_total << " "
            << total_clientes << " "
            << sol.rutas.size() << " ";
    archivo << fixed << setprecision(6) << tiempo_ejecucion << "[s]" << endl;

    for (size_t i = 0; i < sol.rutas.size(); ++i) {
        const Ruta& ruta = sol.rutas[i];
        string ruta_formateada;
        for (size_t j = 0; j < ruta.nodos.size(); ++j) {
            int id = ruta.nodos[j];
            const Nodo& nodo = mapa_nodos.at(id);
            if (nodo.tipo == 0) {
                ruta_formateada += "Base";
            } else if (nodo.tipo == 1) {
                ruta_formateada += "L" + to_string(id);
            } else if (nodo.tipo == 2) {
                ruta_formateada += "B" + to_string(id);
            }
            if (j != ruta.nodos.size() - 1) {
                ruta_formateada += "-";
            }
        }
        archivo << ruta_formateada << " "
                << fixed << setprecision(2) << ruta.costo_total << " "
                << int(ruta.carga_utilizada);
        if (i != sol.rutas.size() - 1) archivo << "\n";
    }

    archivo.close();
}

//=====================GENERACIÓN DE SOLUCIÓN INICIAL ALEATORIA=======================
//====================================================================================

Solucion generar_solucion_inicial(const vector<Nodo>& nodos, int cantidad_vehiculos, float capacidad_vehiculo, int id_deposito) {
    vector<Nodo> linehauls, backhauls;
    unordered_map<int, Nodo> mapa_nodos;
    for (const auto& nodo : nodos) {
        mapa_nodos[nodo.id] = nodo;
        if (nodo.tipo == 1) linehauls.push_back(nodo);
        else if (nodo.tipo == 2) backhauls.push_back(nodo);
    }

    mt19937 rng(time(0));
    shuffle(linehauls.begin(), linehauls.end(), rng);
    shuffle(backhauls.begin(), backhauls.end(), rng);

    Solucion sol;
    sol.rutas.resize(cantidad_vehiculos);
    unordered_set<int> usados;

    size_t next_linehaul = 0, next_backhaul = 0;

    // Iniciar cada ruta con el depósito
    for (int i = 0; i < cantidad_vehiculos; ++i) {
        sol.rutas[i].nodos.push_back(id_deposito);
    }

    // Asignar 1 linehaul a cada camión si es posible
    for (int i = 0; i < cantidad_vehiculos && next_linehaul < linehauls.size(); ++i) {
        Ruta& ruta = sol.rutas[i];
        const Nodo& n = linehauls[next_linehaul++];
        ruta.nodos.push_back(n.id);
        ruta.carga_entregada += n.demanda;
        usados.insert(n.id);
    }

    // (2) Reasignar linehauls restantes sin superar capacidad y realizando una distribución + equitativa 
    bool asignado = true;
    while (asignado && next_linehaul < linehauls.size()) {
        asignado = false;
        for (int i = 0; i < cantidad_vehiculos && next_linehaul < linehauls.size(); ++i) {
            Ruta& ruta = sol.rutas[i];
            const Nodo& n = linehauls[next_linehaul];
            if (!usados.count(n.id) && ruta.carga_entregada + n.demanda <= capacidad_vehiculo) {
                ruta.nodos.push_back(n.id);
                ruta.carga_entregada += n.demanda;
                usados.insert(n.id);
                ++next_linehaul;
                asignado = true;
            }
        }
    }

    // (6a Aseguramos que cada vehículo termina en el deposito)
    for (int i = 0; i < cantidad_vehiculos; ++i) {
        sol.rutas[i].nodos.push_back(id_deposito);
    }

    // (4 y 5) Asignar backhauls a rutas ya con entregas, sin pasar capacidad
    bool backhaul_asignado = true;
    while (backhaul_asignado && next_backhaul < backhauls.size()) {
        backhaul_asignado = false;
        for (int i = 0; i < cantidad_vehiculos && next_backhaul < backhauls.size(); ++i) {
            Ruta& ruta = sol.rutas[i];
            const Nodo& n = backhauls[next_backhaul];

            float capacidad_restante = capacidad_vehiculo - ruta.carga_recogida;

            if (!usados.count(n.id) &&
                ruta.carga_entregada > 0 &&  // (4) no sobrepasar capacidad en recogidas y permitir rutas solo linehaul pero no backhaul
                n.demanda <= capacidad_restante) {

                ruta.nodos.insert(ruta.nodos.end() - 1, n.id); // (9) después de entregas
                ruta.carga_recogida += n.demanda;
                usados.insert(n.id);
                ++next_backhaul;
                backhaul_asignado = true;
            }
        }
    }

    // Acumulado total de carga para validación 
    for (auto& ruta : sol.rutas) {
        ruta.carga_utilizada = ruta.carga_entregada + ruta.carga_recogida;
    }
    calcular_costo_total(sol, mapa_nodos);
    return sol;
}


//=====================MOVIMIENTO SWAP CON CUMPLIMIENTO DE RESTRICCIONES=======================
//=============================================================================================

bool swap_nodos(Solucion& sol, const unordered_map<int, Nodo>& mapa_nodos, float capacidad_vehiculo, int max_intentos = 1000) {
    mt19937 rng(time(0));
    uniform_int_distribution<> distrib_ruta(0, sol.rutas.size() - 1);
    uniform_int_distribution<> distrib_tipo(1, 2); // 1: linehaul, 2: backhaul

    for (int intento = 0; intento < max_intentos; ++intento) {
        int tipo_objetivo = distrib_tipo(rng);

        int idx_ruta1 = distrib_ruta(rng);
        int idx_ruta2 = distrib_ruta(rng);
        while (idx_ruta2 == idx_ruta1) idx_ruta2 = distrib_ruta(rng);

        Ruta& ruta1 = sol.rutas[idx_ruta1];
        Ruta& ruta2 = sol.rutas[idx_ruta2];

        vector<int> candidatos1, candidatos2;

        for (size_t i = 1; i < ruta1.nodos.size() - 1; ++i)
            if (mapa_nodos.at(ruta1.nodos[i]).tipo == tipo_objetivo) candidatos1.push_back(i);
        for (size_t i = 1; i < ruta2.nodos.size() - 1; ++i)
            if (mapa_nodos.at(ruta2.nodos[i]).tipo == tipo_objetivo) candidatos2.push_back(i);

        if (candidatos1.empty() || candidatos2.empty()) continue;

        shuffle(candidatos1.begin(), candidatos1.end(), rng);
        shuffle(candidatos2.begin(), candidatos2.end(), rng);

        for (int i : candidatos1) {
            int id1 = ruta1.nodos[i];
            const Nodo& nodo1 = mapa_nodos.at(id1);

            for (int j : candidatos2) {
                int id2 = ruta2.nodos[j];
                const Nodo& nodo2 = mapa_nodos.at(id2);

                float nueva_entrega1 = ruta1.carga_entregada;
                float nueva_entrega2 = ruta2.carga_entregada;
                float nueva_recogida1 = ruta1.carga_recogida;
                float nueva_recogida2 = ruta2.carga_recogida;

                if (tipo_objetivo == 1) {
                    nueva_entrega1 = nueva_entrega1 - nodo1.demanda + nodo2.demanda;
                    nueva_entrega2 = nueva_entrega2 - nodo2.demanda + nodo1.demanda;
                } else {
                    nueva_recogida1 = nueva_recogida1 - nodo1.demanda + nodo2.demanda;
                    nueva_recogida2 = nueva_recogida2 - nodo2.demanda + nodo1.demanda;
                }

                bool capacidad_ok = (nueva_entrega1 <= capacidad_vehiculo && nueva_entrega2 <= capacidad_vehiculo &&
                                    nueva_recogida1 <= capacidad_vehiculo && nueva_recogida2 <= capacidad_vehiculo);

                if (capacidad_ok) {
                    swap(ruta1.nodos[i], ruta2.nodos[j]);

                    ruta1.carga_entregada = nueva_entrega1;
                    ruta2.carga_entregada = nueva_entrega2;
                    ruta1.carga_recogida = nueva_recogida1;
                    ruta2.carga_recogida = nueva_recogida2;

                    ruta1.carga_utilizada = nueva_entrega1 + nueva_recogida1;
                    ruta2.carga_utilizada = nueva_entrega2 + nueva_recogida2;

                    ruta1.costo_total = calcular_costo_ruta(ruta1, mapa_nodos);
                    ruta2.costo_total = calcular_costo_ruta(ruta2, mapa_nodos);

                    sol.costo_total = 0;
                    for (auto& r : sol.rutas) sol.costo_total += r.costo_total;

                    return true;
                }
            }
        }
    }
    return false;
}


//=====================TÉCNICA: SIMULATED ANNEALING=======================
//========================================================================

Solucion simulated_annealing(Solucion solucion_inicial,
                            const unordered_map<int, Nodo>& mapa_nodos,
                            float capacidad_vehiculo,
                            float temperatura_inicial,
                            float alfa,
                            int iteraciones_por_temperatura,
                            int max_iteraciones,
                            int swaps_por_vecino) 
{
    mt19937 rng(time(0));
    uniform_real_distribution<> prob(0.0, 1.0);

    Solucion sc = solucion_inicial;     // Solución actual
    Solucion sbest = solucion_inicial;  // Mejor solución encontrada
    float T = temperatura_inicial;
    int t = 0;

    while (t < max_iteraciones && T > 10) {
        for (int k = 0; k < iteraciones_por_temperatura; ++k) {
            Solucion candidato = sc;
            bool exito = true;

            for (int s = 0; s < swaps_por_vecino; ++s) {
                if (!swap_nodos(candidato, mapa_nodos, capacidad_vehiculo)) {
                    exito = false;
                    break;
                }
            }

            if (!exito) continue;

            float delta_eval = candidato.costo_total - sc.costo_total;

            if (delta_eval < 0) {
                sc = candidato;
            } else {
                float p = exp(-delta_eval / T);
                if (prob(rng) < p) {
                    sc = candidato;
                }
            }

            if (sc.costo_total < sbest.costo_total) {
                sbest = sc;
            }
        }

        T *= alfa;
        t++;
    }

    return sbest;
}


//=====================MAIN=======================
//================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: ./proyecto <nombre_instancia>" << endl;
        return 1;
    }

    string nombre_instancia = argv[1];
    vector<Nodo> nodos;
    unordered_map<int, Nodo> mapa_nodos;
    int cantidad_vehiculos = 0, id_deposito = -1;
    float capacidad_vehiculo = 0;

    leer_instancia(nombre_instancia + ".txt", nodos, mapa_nodos, cantidad_vehiculos, capacidad_vehiculo, id_deposito);

    clock_t inicio = clock();
    Solucion solucion_inicial = generar_solucion_inicial(nodos, cantidad_vehiculos, capacidad_vehiculo, id_deposito);

    //Borrar
    cout << "\n=== Solución Inicial ===" << endl;
    cout << "Costo total: " << solucion_inicial.costo_total << endl;
    for (size_t i = 0; i < solucion_inicial.rutas.size(); ++i) {
        cout << "Ruta " << i + 1 << ": ";
        for (int id : solucion_inicial.rutas[i].nodos) cout << id << " ";
        cout << "| Entrega: " << solucion_inicial.rutas[i].carga_entregada
            << " | Recogida: " << solucion_inicial.rutas[i].carga_recogida
            << " | Total: " << solucion_inicial.rutas[i].carga_utilizada
            << " | Costo: " << solucion_inicial.rutas[i].costo_total << endl;
    }
    
    //Ajuste de parámetros para testeos (MODIFICABLE PARA PROBAR)
    float T0 = 50000.0;
    float alfa = 0.98;  
    int iteraciones_por_T = 500;
    int max_iteraciones = 500;
    int swaps_por_vecino = 5;

    Solucion solucion_final = simulated_annealing(solucion_inicial, mapa_nodos, capacidad_vehiculo, T0, alfa, iteraciones_por_T, max_iteraciones, swaps_por_vecino);
    clock_t fin = clock();
    double tiempo_ejecucion = double(fin - inicio) / CLOCKS_PER_SEC;

    //borrar
    cout << "\n=== Solución Final (SA) ===" << endl;
    cout << "Costo total: " << solucion_final.costo_total << endl;
    for (size_t i = 0; i < solucion_final.rutas.size(); ++i) {
        cout << "Ruta " << i + 1 << ": ";
        for (int id : solucion_final.rutas[i].nodos) cout << id << " ";
        cout << "| Entrega: " << solucion_final.rutas[i].carga_entregada
            << " | Recogida: " << solucion_final.rutas[i].carga_recogida
            << " | Total: " << solucion_final.rutas[i].carga_utilizada
            << " | Costo: " << solucion_final.rutas[i].costo_total << endl;
    }
    
    escribir_salida(nombre_instancia, solucion_final, mapa_nodos, tiempo_ejecucion);
    return 0;
}
