#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <memory>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <functional>
#include <unordered_map>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

struct Node {
    std::string name;
    bool isFile;
    std::vector<std::shared_ptr<Node>> children;
    double x = 0, y = 0;
};

int rnd(int a, int b) {
    static std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<> dist(a, b);
    return dist(gen);
}

std::string makeName(bool isFile, int idx) {
    return isFile ? "file" + std::to_string(idx) + ".txt" : "dir" + std::to_string(idx);
}

std::shared_ptr<Node> genNode(int depth = 0) {
    auto node = std::make_shared<Node>();
    bool file = (depth > 0 && rnd(0, 1) == 1) || depth == 6;
    node->isFile = file;
    static int fileCnt = 0, dirCnt = 0;
    node->name = makeName(file, file ? ++fileCnt : ++dirCnt) + (file ? "" : "/");
    if (!file) {
        int nChild = rnd(1, 3);
        for (int i = 0; i < nChild; ++i) {
            node->children.push_back(genNode(depth + 1));
        }
    }
    return node;
}

void printTree(const Node& n, std::ostream& out, int indent = 0) {
    out << std::string(indent * 2, ' ') << n.name << "\n";
    for (const auto& child : n.children) {
        printTree(*child, out, indent + 1);
    }
}

int countIndent(const std::string& line) {
    int c = 0;
    while (c + 1 < (int)line.size() && line[c] == ' ' && line[c + 1] == ' ') c += 2;
    return c / 2;
}

std::shared_ptr<Node> parseTree(const std::vector<std::string>& lines) {
    std::shared_ptr<Node> root = nullptr;
    std::vector<std::shared_ptr<Node>> levelNodes;
    for (const auto& raw : lines) {
        int lvl = countIndent(raw);
        std::string name = raw.substr(lvl * 2);
        bool isFile = (name.size() > 0 && name.back() != '/');
        auto cur = std::make_shared<Node>();
        cur->name = name;
        cur->isFile = isFile;
        if (lvl == 0) {
            root = cur;
        } else if (lvl <= (int)levelNodes.size()) {
            auto parent = levelNodes[lvl - 1];
            if (parent) parent->children.push_back(cur);
        }
        if ((int)levelNodes.size() > lvl) levelNodes[lvl] = cur;
        else levelNodes.push_back(cur);
        levelNodes.resize(lvl + 1);
    }
    return root;
}

void layout(Node& node, int depth, double dx, double dy, double& curX) {
    node.y = depth * dy;
    if (node.children.empty()) {
        node.x = curX;
        curX += dx;
    } else {
        for (auto& c : node.children)
            layout(*c, depth + 1, dx, dy, curX);
        double sum = 0;
        for (auto& c : node.children) sum += c->x;
        node.x = sum / node.children.size();
    }
}

void emitPikchr(const std::shared_ptr<Node>& root, std::ostream& out) {
    const double dx = 2.0;
    const double dy = 1.0;
    double curX = 0.0;
    layout(*root, 0, dx, dy, curX);

    std::unordered_map<const Node*, std::string> labels;
    int idCounter = 0;

    std::function<void(const Node&)> defineBlocks = [&](const Node& n) {
        std::string label = "N" + std::to_string(idCounter++);
        labels[&n] = label;
        out << label << ": box \"" << n.name << "\" fit at (" << n.x << "," << -n.y << ")\n";
        for (const auto& c : n.children) defineBlocks(*c);
    };
    defineBlocks(*root);

    std::function<void(const Node&)> drawArrows = [&](const Node& n) {
        for (const auto& c : n.children) {
            out << "arrow from " << labels[&n] << ".s to " << labels[c.get()] << ".n\n";
            drawArrows(*c);
        }
    };
    drawArrows(*root);
}

size_t getFileSize(const std::string& filename) {
    return fs::file_size(filename);
}
int main() {
    using namespace std::chrono;

    const size_t TARGET_TOTAL_BYTES = 100 * 1024 * 1024; // можно изменить на 200 для полной версии
    size_t totalBytes = 0;
    size_t totalPikchrBytes = 0;
    size_t totalHtmlBytes = 0;
    int counter = 0;

    std::vector<long long> timePerDiagramNs;

    while (totalBytes < TARGET_TOTAL_BYTES) {
        Node genRoot{ "C:\\\\", false };
        for (int i = 0; i < 3; ++i)
            genRoot.children.push_back(genNode(1));

            std::ostringstream treeStream;
        printTree(genRoot, treeStream);
        std::string treeText = treeStream.str();
        size_t inputSize = treeText.size();
        totalBytes += inputSize;

        std::string baseName = "tree_" + std::to_string(counter);
        std::string txtName = baseName + ".txt";
        std::ofstream(txtName) << treeText;

        std::istringstream treeIn(treeText);
        std::vector<std::string> lines;
        std::string L;
        while (std::getline(treeIn, L)) if (!L.empty()) lines.push_back(L);
        auto parsed = parseTree(lines);
        if (!parsed) {
            std::cerr << "error\n";
            continue;
        }

        std::string pikchrFile = baseName + ".pikchr";
        std::ostringstream pikchrStream;
        emitPikchr(parsed, pikchrStream);
        std::string pikchrCode = pikchrStream.str();
        std::ofstream(pikchrFile) << pikchrCode;
        size_t pikchrSize = std::filesystem::file_size(pikchrFile);
        totalPikchrBytes += pikchrSize;

        std::string htmlFile = baseName + ".html";
        std::string command = "pikchr " + pikchrFile + " > " + htmlFile;
        auto start = high_resolution_clock::now();
        int ret = std::system(command.c_str());
        auto end = high_resolution_clock::now();
        timePerDiagramNs.push_back(duration_cast<nanoseconds>(end - start).count());

        if (ret != 0) {
            std::cerr << "error pikchr\n";
            continue;
        }

        size_t htmlSize = std::filesystem::file_size(htmlFile);
        totalHtmlBytes += htmlSize;

        std::filesystem::remove(txtName);
        std::filesystem::remove(pikchrFile);
        std::filesystem::remove(htmlFile);

        ++counter;
    }

    long long totalTimeNs = 0;
    for (const auto& t : timePerDiagramNs) totalTimeNs += t;

    std::cout << "Объём входных данных: " << totalBytes / (1024.0 * 1024.0) << " МБ\n";
    std::cout << "Объём Pikchr: " << totalPikchrBytes / (1024.0 * 1024.0) << " МБ\n";
    std::cout << "Объём SVG: " << totalHtmlBytes / (1024.0 * 1024.0) << " МБ\n";
    std::cout << "Соотношение вход/выход: " << (double)totalHtmlBytes  / (double)totalPikchrBytes << "\n";
    std::cout << "Среднее время генерации 1 диаграммы: " << (totalTimeNs / 1000.0) / counter << " мкс\n";
    std::cout << "Среднее время обработки 1 байта входа: " << (totalTimeNs * 1.0) / totalBytes << " нс\n";

    return 0;
}
