#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <sstream>
#include <getopt.h>

std::string removePunctuation(const std::string& word) {
    std::string result;
    for (char c : word) {
        if (std::isalpha(c)) {
            result += c;
        }
    }
    return result;
}

// Función para dividir un texto en palabras
std::vector<std::string> splitText(const std::string &text) {
    std::vector<std::string> words;
    std::string word;
    std::istringstream iss(text);
    while (iss >> word) {
        for (char &c : word) {
            c = std::tolower(c);
        }
        word = removePunctuation(word);
        words.push_back(word);
    }
    return words;
}

// Función para procesar la cantidad de líneas del archivo de texto
void procesarlineas(const std::vector<std::string>& lines, std::map<std::string, int>& wordHistogram, std::mutex& mtx) {
    for (const auto& line : lines) {
        // Dividir cada línea del texto en palabras
        std::vector<std::string> words = splitText(line);

        // Incrementar el conteo de cada palabra en el histograma
        for (const std::string& word : words) {
            std::lock_guard<std::mutex> lock(mtx); // Bloquear el mutex para sincronizar el acceso al mapa
            wordHistogram[word]++;
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> textInMemory;
    std::string fileName = "data/quijote.txt";
    int numThreads = 2;

    // Procesar los argumentos de línea de comandos
    int opt;
    while ((opt = getopt(argc, argv, "f:t:h")) != -1) {
        switch (opt) {
            case 'f':
                fileName = optarg;
                break;
            case 't':
                numThreads = std::stoi(optarg);
                break;
            case 'h':
                std::cout << "Modo de uso: " << argv[0] << " --threads N --file FILENAME [--help]" << std::endl;
                std::cout << " --threads: cantidad de threads a utilizar. Si es 1, entonces ejecuta la versión secuencial." << std::endl;
                std::cout << " --file : archivo a procesar." << std::endl;
                std::cout << " --help : muestra este mensaje y termina." << std::endl;
                return EXIT_SUCCESS;
            default:
                std::cerr << "Uso: " << argv[0] << " [-f archivo] [-t hilos]" << std::endl;
                return EXIT_FAILURE;
        }
    }

    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "No se pudo abrir el archivo." << std::endl;
        return EXIT_FAILURE;
    }

    std::string line;
    while (std::getline(file, line)) {
        textInMemory.push_back(line);
    }
    file.close();

    std::vector<std::thread> threads;
    const int lineasporThread = textInMemory.size()/numThreads;
    std::vector<std::map<std::string, int>> histogramas(numThreads);
    std::mutex mtx; // Mutex para garantizar la sincronización

    for (int i = 0; i < numThreads; i++) {
        int lineaInicial = i * lineasporThread;
        int lineaFinal = (i == numThreads - 1) ? textInMemory.size() : (i + 1) * lineasporThread;
        std::vector<std::string> lines(textInMemory.begin() + lineaInicial, textInMemory.begin() + lineaFinal);
        std::cout << "Thread " << i << " Lineas por thread " << lineaFinal - lineaInicial << " lineas." << std::endl;
        threads.emplace_back([&histogramas, i, lines, &mtx]() {
            procesarlineas(lines, histogramas[i], mtx);
        }); //Aqui le asigno a un thread la funcion procesar lineas
    }

    // Esperar a que los hilos terminen
    for (auto& thread : threads) {
        thread.join();
    }

    // Mutex para garantizar la sincronización al combinar los histogramas de palabras
    std::mutex combinarMtx;

    // Combinar los histogramas de palabras
    std::map<std::string, int> wordHistogram;
    for (const auto& histogram : histogramas) {
        for (const auto& entry : histogram) {
            std::lock_guard<std::mutex> lock(combinarMtx); // Bloquear el mutex para sincronizar el acceso al mapa
            wordHistogram[entry.first] += entry.second;
        }
    }

    // Mostrar el histograma de palabras
    for (const auto& entry : wordHistogram) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }

    return EXIT_SUCCESS;
}