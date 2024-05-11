fn main() {
    // Check if the `cfg.toml` file exists and has been filled out.
    if !std::path::Path::new("src/secrets.rs").exists() {
        panic!("You need to create a `secrets.rs` file with your Wi-Fi credentials! Use `secrets.rs.example` as a template.");
    }
    embuild::espidf::sysenv::output();
}
