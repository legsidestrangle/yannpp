#include "network2.h"

#include <algorithm>
#include <numeric>

#include "common/array3d_math.h"
#include "common/cpphelpers.h"
#include "common/log.h"

network2_t::network2_t(std::initializer_list<network2_t::layer_type> layers):
    layers_(layers)
{ }

void network2_t::train(network2_t::training_data const &data,
                       optimizer_t<data_type> const &optimizer,
                       size_t epochs,
                       size_t minibatch_size) {
    log("Training using %d inputs", data.size());
    // big chunk of data is used for training while
    // small chunk - for validation after some epochs
    const size_t training_size = 5 * data.size() / 6;
    std::vector<size_t> eval_indices(data.size() - training_size);
    // generate indices from 1 to the number of inputs
    std::iota(eval_indices.begin(), eval_indices.end(), training_size);

    for (size_t e = 0; e < epochs; e++) {
        auto indices_batches = batch_indices(training_size, minibatch_size);
        const size_t batches_size = indices_batches.size();

        for (size_t b = 0; b < batches_size; b++) {
            update_mini_batch(data, indices_batches[b], optimizer);
            if (b % (batches_size/4) == 0) { log("Processed batch %d out of %d", b, batches_size); }
        }

        auto result = evaluate(data, eval_indices);
        log("Epoch %d: %d / %d", e, result, eval_indices.size());
    }

    auto result = evaluate(data, eval_indices);
    log("End result: %d / %d", result, eval_indices.size());
}

network2_t::t_d network2_t::feedforward(network2_t::t_d const &a) {
    array3d_t<network2_t::data_type> input(a);
    for (auto &layer: layers_) {
        input = layer->feedforward(input);
    }
    return input;
}

#define INPUT(i) std::get<0>(data[i])
#define RESULT(i) std::get<1>(data[i])

size_t network2_t::evaluate(network2_t::training_data const &data, const std::vector<size_t> &indices) {
    size_t count = 0;
    for (auto i: indices) {
        network2_t::t_d result = feedforward(INPUT(i));
        assert(result.size() == RESULT(i).size());
        if (argmax1d(result) == argmax1d(RESULT(i))) { count++; }
    }
    return count;
}

void network2_t::update_mini_batch(network2_t::training_data const &data,
                                   const std::vector<size_t> &indices,
                                   const optimizer_t<data_type> &strategy) {
    for (auto i: indices) {
        backpropagate(INPUT(i), RESULT(i));
    }

    for (auto &layer: layers_) {
        layer->update_weights(strategy);
    }
}

void network2_t::backpropagate(t_d const &x, t_d const &result) {
    const size_t layers_size = layers_.size();
    array3d_t<network2_t::data_type> input(x);

    // feedforward input
    for (size_t i = 0; i < layers_size; i++) {
        input = layers_[i]->feedforward(input);
    }

    // backpropagate error
    array3d_t<network2_t::data_type> error(result);
    for (size_t i = layers_size; i-- > 0;) {
        error = layers_[i]->backpropagate(error);
    }
}

