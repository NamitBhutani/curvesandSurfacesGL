{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "opengl-dev";

  buildInputs = with pkgs; [
    pkg-config
    glm
    libGL        
    glfw       
    gcc
    cmake
    ninja   
    assimp
  ];
}