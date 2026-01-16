/// Training parameters used to create Renegade's networks
/// bullet version used: e5f65c4 (Measure CPU dataloading throughput utility (#489)) from November 26, 2025

/// Net 33 is trained in 2 stages (keeping the accidental schedule from net 31):
/// - first 600 sb: cosine decay, wdl 0.4 -> 0.6
/// - final 200 sb: continue the cosine decay, wdl fixed at 0.6


#[allow(unused_imports)]
use bullet_lib::{
    game::{
        inputs::{ChessBucketsMirrored, get_num_buckets},
        outputs::MaterialCount,
    },
    nn::{
        InitSettings, Shape,
        optimiser::{AdamW, AdamWParams},
    },
    trainer::{
        save::SavedFormat,
        schedule::{TrainingSchedule, TrainingSteps, lr, wdl},
        settings::LocalSettings,
    },
    value::{
        ValueTrainerBuilder,
        loader::{ViriBinpackLoader, DirectSequentialDataLoader}
    },
};


#[rustfmt::skip]
fn main() {
    
    const NET_ID: &str = "renegade_net_33a";
    const HL_SIZE: usize = 1600;
    const NUM_OUTPUT_BUCKETS: usize = 8;
    const BUCKET_LAYOUT: [usize; 32] = [
         0,  1,  2,  3,
         4,  5,  6,  7,
         8,  8,  9,  9,
        10, 10, 11, 11,
        10, 10, 11, 11,
        12, 12, 13, 13,
        12, 12, 13, 13,
        12, 12, 13, 13,
    ];
    const NUM_INPUT_BUCKETS: usize = get_num_buckets(&BUCKET_LAYOUT);

    let mut trainer = ValueTrainerBuilder::default()
        .dual_perspective()
        .optimiser(AdamW)
        .inputs(ChessBucketsMirrored::new(BUCKET_LAYOUT))
        .output_buckets(MaterialCount::<NUM_OUTPUT_BUCKETS>)
        .save_format(&[
            SavedFormat::id("l0w")
                .transform(|store, weights| {
                    let factoriser = store.get("l0f").values.repeat(NUM_INPUT_BUCKETS);
                    weights.into_iter().zip(factoriser).map(|(a, b)| a + b).collect()
                })
                .round()
                .quantise::<i16>(255),
            SavedFormat::id("l0b").round().quantise::<i16>(255),
            SavedFormat::id("l1w").round().quantise::<i16>(64).transpose(),
            SavedFormat::id("l1b").round().quantise::<i16>(255 * 64),
        ])
        .loss_fn(|output, target| output.sigmoid().squared_error(target))
        .build(|builder, stm_inputs, ntm_inputs, output_buckets| {
            // input layer factoriser
            let l0f = builder.new_weights("l0f", Shape::new(HL_SIZE, 768), InitSettings::Zeroed);
            let expanded_factoriser = l0f.repeat(NUM_INPUT_BUCKETS);

            // input layer weights
            let mut l0 = builder.new_affine("l0", 768 * NUM_INPUT_BUCKETS, HL_SIZE);
            l0.weights = l0.weights + expanded_factoriser;

            // output layer weights
            let l1 = builder.new_affine("l1", 2 * HL_SIZE, NUM_OUTPUT_BUCKETS);

            // inference
            let stm_hidden = l0.forward(stm_inputs).screlu();
            let ntm_hidden = l0.forward(ntm_inputs).screlu();
            let hidden_layer = stm_hidden.concat(ntm_hidden);
            l1.forward(hidden_layer).select(output_buckets)
        });

    // need to account for factoriser weight magnitudes
    let stricter_clipping = AdamWParams { max_weight: 0.99, min_weight: -0.99, ..Default::default() };
    trainer.optimiser.set_params_for_weight("l0w", stricter_clipping);
    trainer.optimiser.set_params_for_weight("l0f", stricter_clipping);

    let schedule = TrainingSchedule {
        net_id: NET_ID.to_string(),
        eval_scale: 400.0,
        steps: TrainingSteps {
            batch_size: 16384,
            batches_per_superbatch: 6104,  // ~100 million positions
            start_superbatch: 1,
            end_superbatch: 800,
        },
        wdl_scheduler: wdl::Sequence {
            first: wdl::LinearWDL { start: 0.4, end: 0.6, },
            second: wdl::ConstantWDL { value: 0.6, },
            first_scheduler_final_superbatch: 600,
        },
        lr_scheduler: lr::Warmup {
            inner: lr::CosineDecayLR {
                initial_lr: 0.001,
                final_lr: 0.001 * f32::powi(0.3, 5),
                final_superbatch: 800,
            },
            warmup_batches: 6104,
        },
        save_rate: 10,
    };

    let settings = LocalSettings { threads: 4, test_set: None, output_directory: "checkpoints", batch_queue_size: 32 };

    let data_loader = DirectSequentialDataLoader::new(
        &["../nnue/data/241010_241213_250418_251202_260112_frc241002"]
    );

    trainer.run(&schedule, &settings, &data_loader);
    
    for fen in [
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "2r3k1/2P2pp1/3Np2p/8/7P/5qP1/5P1K/2Q5 b - - 2 42",
        "rnbqkb1r/pppppppp/8/3nP3/2P5/8/PP1P1PPP/RNBQKBNR b KQkq - 0 3",
        "2rk1b1r/1Qp1p2p/p1P5/5p2/2PP2B1/4B3/5P1P/1R2K3 w - - 0 35",
        "rnbqkb1r/pp2pppp/2p2n2/3p4/2PP4/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 4",
        "r3r1k1/ppp2ppp/3qbb2/2p1N2Q/5P2/1PPB4/P1P3PP/1R3RK1 b - - 2 15",
        "7k/pp4rp/3p1Q2/1P1P4/2Pp4/3Pb2P/2q3P1/5R1K w - - 7 37",
        "8/8/4p3/4P1p1/Pk6/2p5/1p3K2/1r6 b - - 1 52",
        "r2qr1k1/pp2bppp/2n2n2/5b2/2P1N3/3P1N2/PP1BQPPP/R3KB1R w KQ - 2 11",
        "r6k/p1nP3p/n1QP1qp1/1B6/1b6/7P/2P2PP1/1R1R2K1 b - - 0 32",
        "1n3rk1/7p/1q4p1/p1p2pR1/2B2P2/4P3/P1QP1P2/4K3 b - - 1 24",
        "2r4k/5Qp1/p6p/3Np1b1/4P3/P7/1P2n1PP/2r2R1K w - - 1 33",
        "3r4/5p1k/5bpP/8/4K1P1/2p1PN2/8/7R b - - 9 105",
        "r1bqk2r/p1p1bppp/1p2pB2/8/3P4/3B1N2/PPP2PPP/R2QK2R b KQkq - 0 9",
        "1rbqk2r/4ppb1/2np2p1/p1p4p/N1P1P3/4BP1P/PP1Q2P1/2KR1B1R w k - 1 15",
        "5k2/8/4p2p/4P1p1/3P2P1/3b3K/3B2P1/8 w - - 92 121",
        "rnbqk2r/ppppppbp/5np1/8/2PP4/2N2N2/PP2PPPP/R1BQKB1R b KQkq - 3 4",
        "2r1r1k1/p3B2p/3p1P2/2pP3q/P1P3n1/1R3p1P/2Q3P1/1R5K b - - 2 47",
        "8/8/6R1/5K1p/8/5k2/6p1/8 w - - 0 79",
        "3k4/8/1Q6/1b6/8/1P1p1P2/3K4/7q b - - 7 64",
        "6Q1/8/8/7k/8/8/3p1pp1/3Kbrrb w - - 0 1",
        "1rqbkrbn/1ppppp1p/1n6/p1N3p1/8/2P4P/PP1PPPP1/1RQBKRBN w FBfb - 0 9",
        "rbbqn1kr/pp2p1pp/6n1/2pp1p2/2P4P/P7/BP1PPPP1/R1BQNNKR w HAha - 0 9",
        "rqbbk1r1/1ppp2pp/p3n1n1/4pp2/P7/1PP1N3/1Q1PPPPP/R1BB1RKN b ga - 3 10",
        "brnqnbkr/pppppppp/8/8/8/8/PPPPPPPP/BQNRNKRB w GDhb - 0 1"
    ] {
        let eval = trainer.eval(fen);
        println!("FEN: {} -> eval: {}", fen, 400.0 * eval);
    }
}
