#ifndef DISTRIBUTION_HPP
#define DISTRIBUTION_HPP

#include <random>


/*  NOTE : the distribuitons returns the minimum value of FLOOR, and maximum valve of ceiling,
 *  if ceiling is none zero, positive interger, or no upper bound if ceiling is zero.
 */

#define FLOOR 1

class Distribution {
public:
    virtual unsigned int nextRandNum (std::default_random_engine rng) = 0;

protected:
    unsigned int ceiling_ = 0;
};

class Geometric : public Distribution{
public:
    Geometric(std::string params) {

        size_t pos = params.find(",");
        p_ = std::stod(params.substr(0, pos));
        if (p_ < 0 || p_ > 1) {
            std::cerr << "Geometric Distribution: Invalid parameter." << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(params.substr(pos+1));
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::geometric_distribution<int> distribution(p_);
        unsigned int val = (unsigned int) distribution(rng);
        return ((ceiling_) ? val % ceiling_ : val) + FLOOR;
    }

private:
    double p_ = 0.0;
};


class Exponential : public Distribution {
public:
    Exponential(std::string params) {

        size_t pos = params.find(",");
        lambda_  = std::stod(params.substr(0, pos));
        if (lambda_ <= 0 || lambda_ >= 1) {
            std::cerr << "Exponential Distribution: Invalid parameter." << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(params.substr(pos+1));
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::exponential_distribution<double> distribution(lambda_);
        unsigned int val = (unsigned int) distribution(rng);
        return ((ceiling_) ? val % ceiling_ : val) + FLOOR;
    }

private:
    double lambda_ = 0.0;
};


class Binomial : public Distribution {
public:
    Binomial (std::string params) {

        size_t pos = params.find(",");
        p_ = std::stod(params.substr(0, pos));
        if (p_ < 0 || p_ > 1) {
            std::cerr << "Binomial Distribution: Invalid parameter p." << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(params.substr(pos+1));
        if (ceiling_ <= FLOOR) {
            std::cerr << "Binomial Distribution: Invalid ceiling value." << std::endl;
            abort();
        }
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::binomial_distribution<int> distribution(ceiling_-FLOOR, p_);
        unsigned int val = (unsigned int) distribution(rng);
        return val + FLOOR;
    }

private:
    double p_ = 0.0;
};


class Normal : public Distribution {
public:
    Normal (std::string params) {

        size_t pos = params.find(",");
        mean_ = std::stod(params.substr(0, pos));
        std::string temp = params.substr(pos+1);
        pos = temp.find(",");
        stddev_ = std::stod(temp.substr(0,pos));
        if(mean_ < 0 || stddev_ <= 0) {
            std::cerr << "Normal Distribution: Invalid parameter" << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(temp.substr(pos+1));
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::normal_distribution<double> distribution(mean_, stddev_);
        unsigned int val = (unsigned int) distribution(rng);
        return ((ceiling_) ? val % ceiling_ : val) + FLOOR;
    }

private:
    double mean_ = 0.0, stddev_ = 0.0;
};


class Uniform : public Distribution {
public:
    Uniform (std::string params) {

        size_t pos = params.find(",");
        a_ = std::stoul(params.substr(0, pos));
        std::string temp = params.substr(pos+1);
        pos = temp.find(",");
        b_ = std::stoul(temp.substr(0,pos));
        if(a_ >= b_) {
            std::cerr << "Uniform Distribution: Invalid parameter" << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(temp.substr(pos+1));
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::uniform_int_distribution<int> distribution(a_, b_);
        unsigned int val = (unsigned int) distribution(rng);
        return ((ceiling_) ? val % ceiling_ : val) + FLOOR;
    }

private:
    unsigned int a_ = 0, b_ = 0;
};

        
class Poisson : public Distribution {
public:
    Poisson (std::string params) {

        size_t pos = params.find(",");
        mean_ = std::stod(params.substr(0,pos));
        if(mean_ <= 3) { //if mena lower than 3 higher chance return 0 or 1
            std::cerr << "Poisson Distribution: Invalid parameter" << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(params.substr(pos+1));
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::poisson_distribution<int> distribution(mean_);
        unsigned int val = (unsigned int) distribution(rng);
        return ((ceiling_) ? val % ceiling_ : val) + FLOOR;
    }

private:
    double mean_ = 0.0;
};


class Lognormal : public Distribution {
public:
    Lognormal (std::string params) {

        std::string delimiter = ",";
        size_t pos = params.find(delimiter);
        mean_ = std::stod(params.substr(0, pos));
        std::string temp = params.substr(pos+1);
        pos = temp.find(",");
        stddev_ = std::stod(temp.substr(0,pos));
        if(mean_ <= 1 || stddev_ < 1) {
            std::cerr << "Lognormal Distribution: Invalid parameter" << std::endl;
            abort();
        }
        ceiling_ = (unsigned int) std::stoul(temp.substr(pos+1));
    }

    unsigned int nextRandNum(std::default_random_engine rng) {

        std::lognormal_distribution<double> distribution(mean_, stddev_);
        unsigned int val = (unsigned int) distribution(rng);
        return ((ceiling_) ? val % ceiling_ : val) + FLOOR;
    }

private:
    double mean_ = 0.0, stddev_ = 0.0;
};

Distribution *buildDist (std::string params) {

    std::size_t pos = params.find(",");
    std::string distribution_type = params.substr(0, pos);
    params.erase(0, pos+1);

    if (distribution_type == "geometric") {
        return new Geometric(params);

    } else if (distribution_type == "exponential") {
        return new Exponential(params);

    } else if (distribution_type == "binomial") {
        return new Binomial(params);

    } else if (distribution_type == "normal") {
        return new Normal(params);

    } else if (distribution_type == "uniform") {
        return new Uniform(params);

    } else if (distribution_type == "poisson") {
        return new Poisson(params);

    } else if (distribution_type == "lognormal") {
        return new Lognormal(params);

    } else {
        std::cerr << "Invalid choice of distribution." << std::endl;
        abort();
    }
}

#endif
