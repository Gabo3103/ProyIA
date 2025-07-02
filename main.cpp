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

// Lectura de la instancia con su formato
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

// Escritura de salida con su formato
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

// Movimiento Relocate
bool relocate(Solucion& sol, const unordered_map<int, Nodo>& mapa_nodos, float capacidad_vehiculo, int id_deposito) {
    mt19937 rng(time(0));
    uniform_int_distribution<> distrib_ruta(0, sol.rutas.size() - 1);

    int origen_idx = distrib_ruta(rng);
    int destino_idx = distrib_ruta(rng);
    while (destino_idx == origen_idx) destino_idx = distrib_ruta(rng);

    Ruta& ruta_origen = sol.rutas[origen_idx];
    Ruta& ruta_destino = sol.rutas[destino_idx];

    vector<int> candidatos;
    for (size_t i = 1; i < ruta_origen.nodos.size() - 1; ++i) {
        int id = ruta_origen.nodos[i];
        if (mapa_nodos.at(id).tipo != 0) candidatos.push_back(i);
    }
    if (candidatos.empty()) return false;

    uniform_int_distribution<> distrib_nodo(0, candidatos.size() - 1);
    int idx_en_origen = candidatos[distrib_nodo(rng)];
    int id_nodo = ruta_origen.nodos[idx_en_origen];
    const Nodo& nodo = mapa_nodos.at(id_nodo);

    // 🔍 Depuración inicial
    std::cout << "\n🔄 Intentando mover Nodo " << id_nodo
            << " (" << (nodo.tipo == 1 ? "Linehaul" : "Backhaul") << ") "
            << "de Ruta " << origen_idx + 1 << " a Ruta " << destino_idx + 1 << "\n";

    std::cout << "Ruta origen antes: ";
    for (int id : ruta_origen.nodos) std::cout << id << " ";
    std::cout << "\nRuta destino antes: ";
    for (int id : ruta_destino.nodos) std::cout << id << " ";
    std::cout << "\n";

    float costo_antes = sol.costo_total;

    // Remover el nodo de la ruta origen
    ruta_origen.nodos.erase(ruta_origen.nodos.begin() + idx_en_origen);
    if (nodo.tipo == 1) ruta_origen.carga_entregada -= nodo.demanda;
    else ruta_origen.carga_recogida -= nodo.demanda;

    size_t insert_pos = 1;
    if (nodo.tipo == 2) {
        while (insert_pos + 1 < ruta_destino.nodos.size()) {
            int tipo = mapa_nodos.at(ruta_destino.nodos[insert_pos]).tipo;
            if (tipo == 2) break;
            insert_pos++;
        }
    }

    float nueva_entrega = ruta_destino.carga_entregada;
    float nueva_recogida = ruta_destino.carga_recogida;
    if (nodo.tipo == 1) nueva_entrega += nodo.demanda;
    else nueva_recogida += nodo.demanda;

    if (nueva_entrega + nueva_recogida <= capacidad_vehiculo) {
        ruta_destino.nodos.insert(ruta_destino.nodos.begin() + insert_pos, id_nodo);
        ruta_destino.carga_entregada = nueva_entrega;
        ruta_destino.carga_recogida = nueva_recogida;

        ruta_origen.carga_utilizada = ruta_origen.carga_entregada + ruta_origen.carga_recogida;
        ruta_destino.carga_utilizada = ruta_destino.carga_entregada + ruta_destino.carga_recogida;

        ruta_origen.costo_total = calcular_costo_ruta(ruta_origen, mapa_nodos);
        ruta_destino.costo_total = calcular_costo_ruta(ruta_destino, mapa_nodos);

        sol.costo_total = 0;
        for (auto& r : sol.rutas) sol.costo_total += r.costo_total;

        float costo_despues = sol.costo_total;
        float delta = costo_despues - costo_antes;

        // ✅ Confirmación y estado final
        std::cout << "✅ Movimiento realizado exitosamente.\n";
        std::cout << "Ruta origen después: ";
        for (int id : ruta_origen.nodos) std::cout << id << " ";
        std::cout << "\nRuta destino después: ";
        for (int id : ruta_destino.nodos) std::cout << id << " ";
        std::cout << "\nDelta de costo: " << delta << "\n";

        return true;
    } else {
        std::cout << "❌ Movimiento rechazado por capacidad.\n";
        return false;
    }
}



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
    Solucion sol = generar_solucion_inicial(nodos, cantidad_vehiculos, capacidad_vehiculo, id_deposito);
    relocate(sol, mapa_nodos, capacidad_vehiculo, id_deposito);
    clock_t fin = clock();
    double tiempo_ejecucion = double(fin - inicio) / CLOCKS_PER_SEC;

    cout << "\n=== Solucion Inicial ===" << endl;
    cout << "Costo total: " << sol.costo_total << endl;

    for (size_t i = 0; i < sol.rutas.size(); ++i) {
        cout << "Ruta " << i + 1 << ": ";
        for (int id : sol.rutas[i].nodos) {
            cout << id << " ";
        }
        cout << "| Entrega: " << sol.rutas[i].carga_entregada;
        cout << " | Recolección: " << sol.rutas[i].carga_recogida;
        cout << " | Total: " << sol.rutas[i].carga_utilizada;
        cout << " | Costo: " << sol.rutas[i].costo_total << endl;
    }
    escribir_salida(nombre_instancia, sol, mapa_nodos, tiempo_ejecucion);

    return 0;
}
//SOLUCION SA: MOVIMIENTO, TEMPERATURA, SECUENCIA DE N ALEATORIOS, CALCULO DELTA EVAL Y FUNCIÓN DE ACEPTACIÓN