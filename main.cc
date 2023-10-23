#include "global.hh"
#include "checkArgs.hpp"

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
    ArgsHandler args(argc, argv);

    if (args.shouldShowHelp()) {
        std::cout << "Modo de uso: ./histograma_mt --threads N --file FILENAME [--help]\n--threads: cantidad de threads a utilizar. Si es 1, entonces ejecuta la versión secuencial.\n--file: archivo a procesar.\n--help: muestra este mensaje y termina." << std::endl;
        return EXIT_SUCCESS;
    }

    int numThreads = args.getNumThreads();
    std::string filename = args.getFilename();

    if (filename.empty()) {
        std::cerr << "No se proporcionó un nombre de archivo (-f)." << std::endl;
        std::cout << "Modo de uso: ./histograma_mt --threads N --file FILENAME [--help]\n--threads: cantidad de threads a utilizar. Si es 1, entonces ejecuta la versión secuencial.\n--file: archivo a procesar.\n--help: muestra este mensaje y termina." << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> textInMemory;
    try {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "No se pudo abrir el archivo." << std::endl;
            return EXIT_FAILURE;
        }

        std::string line;
        while (std::getline(file, line)) {
            textInMemory.push_back(line);
        }
        file.close();
    } catch (const std::exception& e) {
        std::cerr << "Error al procesar el archivo: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Threads para usar " << numThreads << " threads." << std::endl;
    
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