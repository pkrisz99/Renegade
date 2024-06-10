/// Training parameters used to create Renegade's networks
/// bullet version used: 630c962 from April 18, 2024

use bullet_lib::{
    inputs, outputs, Activation, LocalSettings, LrScheduler,
    TrainerBuilder, TrainingSchedule, WdlScheduler, Loss
};

macro_rules! net_id {
    () => {
        "renegade-net-21"
    };
}
const NET_ID: &str = net_id!();


fn main() {
    let mut trainer = TrainerBuilder::default()
        .quantisations(&[255, 64])
        .input(inputs::Chess768)
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
        wdl_scheduler: WdlScheduler::Linear {
            start: 0.2,
            end: 0.4,
        },
        lr_scheduler: LrScheduler::Step {
            start: 0.001,
            gamma: 0.3,
            step: 120,
        },
        loss_function: Loss::SigmoidMSE,
        save_rate: 10,
    };
	
	
    let settings = LocalSettings {
        threads: 6,
        data_file_paths: vec![
			"../nnue/data/240221_240325_240421_240609_frc240513",
		],
        output_directory: "checkpoints",
    };

    trainer.run(&schedule, &settings);
}
