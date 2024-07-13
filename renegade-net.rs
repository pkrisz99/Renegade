/// Training parameters used to create Renegade's networks
/// bullet version used: 045b896 from June 21, 2024

use bullet_lib::{
    inputs, outputs, lr, wdl, Activation, LocalSettings,
    TrainerBuilder, TrainingSchedule, Loss, optimiser
};

macro_rules! net_id {
    () => {
        "renegade-net-22-hm"
    };
}
const NET_ID: &str = net_id!();


fn main() {
    let mut trainer = TrainerBuilder::default()
		.optimiser(optimiser::AdamW)
        .quantisations(&[255, 64])
        .input(inputs::ChessBucketsMirrored::new([
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
        ]))
        .output_buckets(outputs::Single)
        .feature_transformer(1024)
        .activate(Activation::SCReLU)
        .add_layer(1)
        .build();  //trainer.load_from_checkpoint("checkpoints/testnet");
	
	let schedule = TrainingSchedule {
        net_id: NET_ID.to_string(),
		batch_size: 16384,
        eval_scale: 400.0,
        ft_regularisation: 0.0,
        batches_per_superbatch: 6104,  // ~100 million positions
        start_superbatch: 1,
        end_superbatch: 520,
        wdl_scheduler: wdl::LinearWDL {
            start: 0.2,
            end: 0.4,
        },
        lr_scheduler: lr::StepLR {
            start: 0.001,
            gamma: 0.3,
            step: 120,
        },
        loss_function: Loss::SigmoidMSE,
        save_rate: 40,
		optimiser_settings: optimiser::AdamWParams { decay: 0.01 },
    };
	
	
    let settings = LocalSettings {
        threads: 6,
        data_file_paths: vec![
			"../nnue/data/240221_240325_240421_240609_frc240513",
		],
        output_directory: "checkpoints",
    };

    trainer.run(&schedule, &settings);
	
	for fen in [
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "rn3r2/pbppq1p1/1p2pN1k/4N3/3P4/3B4/PPP2PPP/R3K2R w KQ - 1 13"
    ] {
        let eval = trainer.eval(fen);
        println!("FEN: {fen}");
        println!("-> eval: {}", 400.0 * eval);
    }
}
