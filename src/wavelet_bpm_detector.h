#include "daubechies8.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <vector>

/**
 * Class <code>WaveletBPMDetector</code> can be used to
 * detect the tempo of a track in beats-per-minute.
 * The class implements the algorithm presented by
 * Tzanetakis, Essl and Cookin the paper titled
 * "Audio Analysis using the Discrete Wavelet Transform"
 *
 * Objects of the class can be created using the companion
 * object's factory method.
 *
 * To detect the tempo the discrete wavelet transform is used.
 * Track samples are divided into windows of frames.
 * For each window data are divided into 4 frequency sub-bands
 * through DWT. For each frequency sub-band an envelope is
 * estracted from the detail coffecients by:
 * 1) Full wave rectification (take the absolute value),
 * 2) Downsampling of the coefficients,
 * 3) Normalization (via mean removal)
 * These 4 sub-band envelopes are then summed together.
 * The resulting collection of data is then autocorrelated.
 * Peaks in the correlated data correspond to peaks in the
 * original signal.
 * then peaks are identified on the filtered data.
 * Given the position of such a peak the approximated
 * tempo of the window is computed and appended to a colletion.
 * Once all windows in the track are processed the beat-per-minute
 * value is returned as the median of the windows values.
 **/
class WaveletBPMDetector {
public:
    WaveletBPMDetector(double rate) { sampleRate = rate; }

    /**
     * Given <code>windowFrames</code> samples computes a BPM
     * value for the window and pushes it in <code>instantBpm</code>
     * @param An array of <code>windowFrames</code> samples representing the window
     **/
    double computeWindowBpm(std::vector<double>& data)
    {
        int levels = 4;
        int pace = std::exp2(levels - 1);
        double maxDecimation = pace;
        int minIndex = (int)(60.0 / 220.0 * sampleRate / maxDecimation);
        int maxIndex = (int)(60.0 / 40.0 * sampleRate / maxDecimation);

        Daubechies8 wavelet;
        std::vector<decomposition> decomp = wavelet.decompose(data, levels);
        int dCMinLength = int(decomp[0].second.size() / maxDecimation) + 1;
        std::vector<double> dCSum(dCMinLength);
        std::vector<double> dC;

        // 4 Level DWT
        for (int loop = 0; loop < levels; ++loop) {
            // Extract envelope from detail coefficients
            //  1) Undersample
            //  2) Absolute value
            //  3) Subtract mean
            dC = undersample(decomp[loop].second, pace);
            dC = abs(dC);
            dC = normalize(dC);

            // Recombine detail coeffients
            if (loop == 0) {
                dCSum = dC;
            } else {
                add(dCSum, dC);
            }

            pace >>= 1;
        }

        // Add the last approximated data
        std::vector<double> aC = abs(decomp[levels - 1].first);
        aC = normalize(aC);
        add(dCSum, aC);

        // Autocorrelation
        std::vector<double> correlated = correlate(dCSum);
        std::vector<double> correlatedTmp(
            correlated.begin() + minIndex, correlated.begin() + maxIndex);

        // Detect peak in correlated data
        int location = detectPeak(correlatedTmp);

        // Compute window BPM given the peak
        int realLocation = minIndex + location;
        return 60.0 / realLocation * (sampleRate / maxDecimation);
    }

private:
    double sampleRate;

    /**
     * Identifies the location of data with the maximum absolute
     * value (either positive or negative). If multiple data
     * have the same absolute value the last positive is taken
     * @param data the input array from which to identify the maximum
     * @return the index of the maximum value in the array
     **/
    int detectPeak(std::vector<double>& data)
    {
        double max = DBL_MIN;

        for (double x : data) {
            max = std::max(max, std::abs(x));
        }

        for (unsigned int i = 0; i < data.size(); ++i) {
            if (data[i] == max) {
                return i;
            }
        }

        for (unsigned int i = 0; i < data.size(); ++i) {
            if (data[i] == -max) {
                return i;
            }
        }

        return -1;
    }

    std::vector<double> undersample(std::vector<double>& data, int pace)
    {
        int length = data.size();
        std::vector<double> result(length / pace);
        for (int i = 0, j = 0; i < length; ++i, j += pace) {
            result[i] = data[i];
        }
        return result;
    }

    std::vector<double> abs(std::vector<double>& data)
    {
        for (double& value : data) {
            value = std::abs(value);
        }
        return data;
    }

    std::vector<double> normalize(std::vector<double>& data)
    {
        double mean = std::accumulate(data.begin(), data.end(), 0) / (double)data.size();
        for (double& value : data) {
            value -= mean;
        }
        return data;
    }

    void add(std::vector<double>& data, std::vector<double>& plus)
    {
        for (unsigned int i = 0; i < data.size(); ++i) {
            data[i] += plus[i];
        }
    }

    std::vector<double> correlate(std::vector<double>& data)
    {
        int n = data.size();
        std::vector<double> correlation(n, 0);
        for (int k = 0; k < n; ++k) {
            for (int i = 0; i < n; ++i) {
                if (k + i < n) {
                    correlation[k] += data[i] * data[k + i];
                }
            }
        }
        return correlation;
    }
};
