#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;
struct DataRow {
    int userId;
    int movieId;
    double rating;
    std::string timestamp;
    DataRow* next;

    DataRow(int uid, int mid, double r, const std::string& ts) : userId(uid), movieId(mid), rating(r), timestamp(ts), next(nullptr) {}
};
void printData(const DataRow* head) {
    const DataRow* current = head;
    while (current) {
        std::cout << "userId: " << current->userId << ", movieId: " << current->movieId << ", rating: " << current->rating << ", timestamp: " << current->timestamp << std::endl;
        current = current->next;
    }
}
void printMatrix(const std::vector<std::vector<double>>& dataMatrix) {
    for (size_t i = 0; i < dataMatrix.size(); ++i) {
        for (size_t j = 0; j < dataMatrix[i].size(); ++j) {
            std::cout << dataMatrix[i][j] << "\t";
        }
        std::cout << std::endl;
    }
}

double averageOfUserId(const std::vector<std::vector<double>>& dataMatrix, int i) {
    double sum = 0;
    int count = 0;
    for (size_t j = 0; j < dataMatrix[i].size(); j++) {
        if (dataMatrix[i][j] != 0.0) {
            sum += dataMatrix[i][j];
            count++;
        }
    }
    return count > 0 ? sum / count : 0.0;
}
int findMaxItems(const std::vector<std::vector<double>>& dataMatrix) {
    int max_items = 0;

    for (const auto& user : dataMatrix) {
        int n_items = user.size();
        if (n_items > max_items) {
            max_items = n_items;
        }
    }

    return max_items;
}
void normalizeMatrix(vector<vector<double>>& dataMatrix) {
    int max_items = findMaxItems(dataMatrix);
    for (size_t i = 0; i < dataMatrix.size(); i++) {
        double userAverage = averageOfUserId(dataMatrix, i);

        for (size_t j = 0; j < max_items; j++) {
            if (j >= dataMatrix[i].size()) {
                dataMatrix[i].push_back(0.0);
            }

            if (dataMatrix[i][j] != 0.0) {
                dataMatrix[i][j] -= userAverage;
            }
            else {
                dataMatrix[i][j] = 0.0;
            }
        }
    }
}

void buildusersimilaritymatrix(vector<vector<double>>& usersimilaritymatrix, vector<vector<double>>& dataMatrix) {
    int n_users = dataMatrix.size();
    usersimilaritymatrix.resize(n_users, vector<double>(n_users, 0.0));

    for (int u = 0; u < n_users; u++) {
        for (int v = u; v < n_users; v++) {
            double dotProduct = 0.0;
            double normU = 0.0;
            double normV = 0.0;

            // Get the actual sizes of normalized rows for users u and v
            int sizeU = dataMatrix[u].size();
            int sizeV = dataMatrix[v].size();
            int max_items = std::max(sizeU, sizeV);

            for (int j = 0; j < max_items; j++) {
                if (j < sizeU && j < sizeV) {
                    dotProduct += dataMatrix[u][j] * dataMatrix[v][j];
                    normU += pow(dataMatrix[u][j], 2);
                    normV += pow(dataMatrix[v][j], 2);
                }
            }

            normU = sqrt(normU);
            normV = sqrt(normV);

            if (normU != 0 && normV != 0) {
                usersimilaritymatrix[u][v] = dotProduct / (normU * normV);
                usersimilaritymatrix[v][u] = usersimilaritymatrix[u][v]; // Since similarity is symmetric
            }
            else {
                usersimilaritymatrix[u][v] = 0.0;
                usersimilaritymatrix[v][u] = 0.0;
            }
        }
    }
}

double predictRatingForItem(const vector<vector<double>>& userSimilarityMatrix,
    const vector<vector<double>>& dataMatrix,
    int targetUserIndex,
    int itemIndex,
    int k) {
    // Check if the target user has already rated the item
    if (dataMatrix[targetUserIndex][itemIndex] != 0.0) {
        cout << "Target user has already rated the item." << endl;
        return dataMatrix[targetUserIndex][itemIndex];
    }

    vector<int> usersWithItem; // To store the indices of users who have rated the item
    for (size_t u = 0; u < dataMatrix.size(); u++) {
        if (dataMatrix[u][itemIndex] != 0.0) {
            usersWithItem.push_back(u);
        }
    }

    double numerator = 0.0;
    double denominator = 0.0;

    // Find the top k users with the highest similarity who have rated the item
    vector<pair<double, int>> topSimilarUsers; // Pair of (similarity, userIndex)
    for (int u : usersWithItem) {
        if (u != targetUserIndex) {
            double similarity = userSimilarityMatrix[targetUserIndex][u];
            topSimilarUsers.push_back(make_pair(similarity, u));
        }
    }
    sort(topSimilarUsers.begin(), topSimilarUsers.end(), greater<pair<double, int>>());

    int count = 0; 
    for (int i = 0; i < topSimilarUsers.size(); i++) {
        double similarity = topSimilarUsers[i].first;
        int userIndex = topSimilarUsers[i].second;
        double rating = dataMatrix[userIndex][itemIndex];

        // Check if the user has rated the item, and use only those users who have rated the item
        if (rating != 0.0) {
            numerator += similarity * rating;
            denominator += abs(similarity);
            count++;

            if (count == k) {
                break; 
            }
        }
    }
    double predictedRating = (denominator != 0.0) ? numerator / denominator : 0.0;
    return predictedRating;
}

void suggestMoviesForUser(const vector<vector<double>>& userSimilarityMatrix,
    const vector<vector<double>>& dataMatrix,
    int targetUserIndex, int k) {
    vector<pair<double, int>> movieRatings; // Pair of (predictedRating, itemIndex)

    // Find movies that user 0 has not rated yet
    for (size_t itemIndex = 0; itemIndex < dataMatrix[0].size(); itemIndex++) {
        if (dataMatrix[targetUserIndex][itemIndex] == 0.0) {
            double predictedRating = predictRatingForItem(userSimilarityMatrix, dataMatrix, targetUserIndex, itemIndex, k);
            movieRatings.push_back(make_pair(predictedRating, itemIndex));
        }
    }

    sort(movieRatings.rbegin(), movieRatings.rend()); // lớn nhất đầu tiên

    cout << "Top " << k << " movie suggestions for user :" << endl;
    for (int i = 0; i < k && i < movieRatings.size(); i++) {
        int itemIndex = movieRatings[i].second;
        double predictedRating = movieRatings[i].first;
        cout << "Movie Index: " << itemIndex << ", Predicted Rating after Normalize: " << predictedRating << endl;
    }
}
void printRow(const vector<double>& row) {
    for (size_t j = 0; j < row.size(); j++) {
        cout << row[j] << "\t";
    }
    cout << endl;
}

int main() {
    std::ifstream file("C:\\Users\\Phong Vu\\OneDrive - Hanoi University of Science and Technology\\DSA\\BTL\\ml-latest-small\\some_handled_train_data.csv");
    if (!file.is_open()) {
        std::cerr << "Failed to open the CSV file." << std::endl;
        return 1;
    }

    std::string line;
    std::getline(file, line);  

    DataRow* head = nullptr;
    DataRow* tail = nullptr;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string value;
        int userId, movieId;
        double rating;
        std::string timestamp;
        std::getline(iss, value, ',');
        userId = std::stoi(value);

        std::getline(iss, value, ',');
        movieId = std::stoi(value);

        std::getline(iss, value, ',');
        rating = std::stod(value);

        std::getline(iss, timestamp, ',');

        // Create a new DataRow instance and add it to the linked list
        DataRow* newRow = new DataRow(userId, movieId, rating, timestamp);
        if (head == nullptr) {
            head = newRow;
            tail = newRow;
        }
        else {
            tail->next = newRow;
            tail = newRow;
        }
    }

    file.close();

    /*  printData(head);*/

    std::vector<std::vector<double>> dataMatrix;

    DataRow* current = head;
    while (current) {
        // Assuming user IDs and movie IDs start from 0
        int userIndex = current->userId;
        int movieIndex = current->movieId;
        double rating = current->rating;

        // Make sure the data matrix is large enough to hold the current user and movie
        if (userIndex >= dataMatrix.size()) {
            dataMatrix.resize(userIndex + 1);
        }
        if (movieIndex >= dataMatrix[userIndex].size()) {
            dataMatrix[userIndex].resize(movieIndex + 1, 0.0);
        }

        // Store the rating in the data matrix
        dataMatrix[userIndex][movieIndex] = rating;

        current = current->next;
    }
    int targetUserIndex=21; 
    int itemIndex=1018; 
    int k=2 ;
    //cin >>  targetUserIndex >> itemIndex >> k;
    double userAverageBeforeNormalization = averageOfUserId(dataMatrix, targetUserIndex);
    cout << userAverageBeforeNormalization << endl;
    vector<vector<double>> usersimilaritymatrix;
    normalizeMatrix(dataMatrix);
    buildusersimilaritymatrix(usersimilaritymatrix, dataMatrix);
 /*   printRow(dataMatrix[0]);*/
    //printRow(usersimilaritymatrix[55]);
    vector<int> usersWithItem;
    for (int u = 0; u < dataMatrix.size(); u++) {
        if (dataMatrix[u][itemIndex] != 0.0) {
            usersWithItem.push_back(u);
        }
    }
    // Predict rating for item 1015 for user 55
    double predictedRating = predictRatingForItem(usersimilaritymatrix, dataMatrix, targetUserIndex, itemIndex, k);
    double originalRating = predictedRating + userAverageBeforeNormalization;
  
    cout << "Predicted rating for User   and Item ID : " << originalRating << endl;
  
    // Find and display the top k most similar users to user 55, considering only users who have rated item 1015
    vector<pair<double, int>> topSimilarUsers; // Pair of (similarity, userIndex)
    for (int u : usersWithItem) {
        if (u != targetUserIndex) {
            double similarity = usersimilaritymatrix[targetUserIndex][u];
            topSimilarUsers.push_back(make_pair(similarity, u));
        }
    }
    sort(topSimilarUsers.begin(), topSimilarUsers.end(), greater<pair<double, int>>());

    cout << "Top " << k << " users with highest similarity to User ID" <<targetUserIndex<<"(who have rated item "<<itemIndex<< endl;
    for (int i = 0; i < k && i < topSimilarUsers.size(); i++) {
        double similarity = topSimilarUsers[i].first;
        int userIndex = topSimilarUsers[i].second;
        cout << "User ID: " << userIndex << ", Similarity Value: " << similarity << endl;
    }
    suggestMoviesForUser(usersimilaritymatrix, dataMatrix, targetUserIndex, k);
    
    
    while (current) {
        DataRow* temp = current;
        current = current->next;
        delete temp;
    }
 
    return 0;
}