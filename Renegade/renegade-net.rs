/// Training parameters used to create Renegade's networks

use bullet::{
    inputs, outputs, Activation, LocalSettings, LrScheduler, TrainerBuilder, TrainingSchedule, WdlScheduler,
};

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
        net_id: "renegade-net-18".to_string(),
        batch_size: 16384,
        eval_scale: 400.0,
        start_epoch: 1,
        end_epoch: 23,
        wdl_scheduler: WdlScheduler::Linear {
            start: 0.3,
            end: 0.6,
        },
        lr_scheduler: LrScheduler::Step {
            start: 0.001,
            gamma: 0.1,
            step: 9,
        },
        save_rate: 1,
    };

    let settings = LocalSettings {
        threads: 4,
        data_file_paths: vec!["../nnue/data/20240221_20240325_20240421"],
        output_directory: "checkpoints",
    };

    trainer.run(&schedule, &settings);
}
