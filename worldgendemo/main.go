package main

import (
	"fmt"
	"image"
	"image/color"
	"image/png"
	"math"
	"math/rand"
	"net/http"
	"strconv"
)

// Material IDs
const (
	Air = iota
	Stone
)

func main() {
	http.HandleFunc("/", serveUI)
	http.HandleFunc("/generate", serveMap)

	fmt.Println("🚀 Live preview server running!")
	fmt.Println("👉 Open http://localhost:8080 in your web browser.")
	if err := http.ListenAndServe(":8080", nil); err != nil {
		panic(err)
	}
}

// --- Web Handlers ---

func serveUI(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html")
	w.Write([]byte(htmlUI))
}

func serveMap(w http.ResponseWriter, r *http.Request) {
	width := 6400
	height := 1800
	surfaceRatio := 0.4

	blockSize := getFloatParam(r, "blockSize", 4.0)
	caIterations := getIntParam(r, "caIterations", 2)
	enablePitfalls := getBoolParam(r, "enablePitfalls", true)
	enableEntrances := getBoolParam(r, "enableEntrances", true)
	crustThickness := getFloatParam(r, "crustThickness", 35.0)

	worldMap := generateWorld(width, height, surfaceRatio, blockSize, enablePitfalls, enableEntrances, crustThickness)

	if caIterations > 0 {
		applyCellularAutomata(worldMap, width, height, caIterations, int(blockSize))
	}

	img := image.NewRGBA(image.Rect(0, 0, width, height))
	colorAir := color.RGBA{R: 20, G: 25, B: 35, A: 255}
	colorStone := color.RGBA{R: 90, G: 80, B: 80, A: 255}
	colorSurface := color.RGBA{R: 50, G: 160, B: 70, A: 255}

	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			if worldMap[y][x] == Air {
				img.Set(x, y, colorAir)
			} else {
				if y > 0 && worldMap[y-1][x] == Air {
					img.Set(x, y, colorSurface)
				} else {
					img.Set(x, y, colorStone)
				}
			}
		}
	}

	w.Header().Set("Content-Type", "image/png")
	png.Encode(w, img)
}

// --- Generator Logic ---

func generateWorld(width, height int, surfaceRatio, blockSize float64, enablePitfalls, enableEntrances bool, avgCrust float64) [][]uint8 {
	world := make([][]uint8, height)
	for i := range world {
		world[i] = make([]uint8, width)
	}

	perlin := NewPerlin(123456789)
	surfaceBaseY := float64(height) * surfaceRatio

	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			fx := float64(x)
			fy := float64(y)

			surfaceY := surfaceBaseY +
				(perlin.Noise2D(fx*0.0008, 0) * 120.0) +
				(perlin.Noise2D(fx*0.005, 0) * 10.0)

			if fy < surfaceY {
				world[y][x] = Air
				continue
			}

			depth := fy - surfaceY

			qx := math.Floor(fx/blockSize) * blockSize
			qy := math.Floor(fy/blockSize) * blockSize

			// Static uniform tunnel thickness everywhere (0.04 is a good middle ground)
			tunnelNoise := perlin.FBm(qx*0.015, qy*0.04, 2)
			tunnelThickness := 0.04
			isAir := math.Abs(tunnelNoise) < tunnelThickness

			if enablePitfalls {
				pitfallNoise := perlin.FBm(qx*0.036, qy*0.005, 2)
				if math.Abs(pitfallNoise) < 0.015 {
					isAir = true
				}
			}

			// Static uniform cavern rarity everywhere
			cavernNoise := perlin.FBm(qx*0.02, qy*0.02, 2)
			if cavernNoise > 0.75 {
				isAir = true
			}

			crustNoiseValue := (perlin.Noise2D(fx*0.001, 100) + 1.0) / 2.0
			crustCurrentThickness := (avgCrust - 15.0) + (crustNoiseValue * 30.0)

			if depth < crustCurrentThickness {
				if enableEntrances {
					entranceNoise := perlin.Noise2D(qx*0.003, qy*0.02)
					if math.Abs(entranceNoise) < 0.03 {
						isAir = true
					} else {
						isAir = false
					}
				} else {
					isAir = false
				}
			}

			if isAir {
				world[y][x] = Air
			} else {
				world[y][x] = Stone
			}
		}
	}
	return world
}

func applyCellularAutomata(world [][]uint8, width, height int, iterations int, blockSize int) {
	if blockSize < 1 {
		blockSize = 1
	}

	for i := 0; i < iterations; i++ {
		buffer := make([][]uint8, height)
		for y := 0; y < height; y++ {
			buffer[y] = make([]uint8, width)
			copy(buffer[y], world[y]) // Copy current state
		}

		// Step by blockSize instead of 1 pixel
		for y := 0; y < height; y += blockSize {
			for x := 0; x < width; x += blockSize {

				// Don't run CA on the high sky
				if world[y][x] == Air && y < int(float64(height)*0.4)-150 {
					continue
				}

				wallCount := 0

				// Check neighboring blocks
				for dy := -1; dy <= 1; dy++ {
					for dx := -1; dx <= 1; dx++ {
						if dy == 0 && dx == 0 {
							continue
						}

						nx := x + (dx * blockSize)
						ny := y + (dy * blockSize)

						// Borders count as solid stone
						if nx < 0 || ny < 0 || nx >= width || ny >= height {
							wallCount++
						} else if world[ny][nx] == Stone {
							wallCount++
						}
					}
				}

				// Determine new state based on block neighbors
				newState := world[y][x]
				if world[y][x] == Stone {
					if wallCount < 4 {
						newState = Air // Erase isolated dots
					}
				} else {
					if wallCount >= 5 {
						newState = Stone // Fill isolated holes
					}
				}

				// Apply the new state to the entire block to keep the chunky look
				for by := 0; by < blockSize; by++ {
					for bx := 0; bx < blockSize; bx++ {
						if y+by < height && x+bx < width {
							buffer[y+by][x+bx] = newState
						}
					}
				}
			}
		}

		// Swap buffers
		for y := 0; y < height; y++ {
			copy(world[y], buffer[y])
		}
	}
}

// --- Helper Functions ---

func getFloatParam(r *http.Request, key string, def float64) float64 {
	val := r.URL.Query().Get(key)
	if val == "" {
		return def
	}
	f, err := strconv.ParseFloat(val, 64)
	if err != nil {
		return def
	}
	return f
}

func getIntParam(r *http.Request, key string, def int) int {
	val := r.URL.Query().Get(key)
	if val == "" {
		return def
	}
	i, err := strconv.Atoi(val)
	if err != nil {
		return def
	}
	return i
}

func getBoolParam(r *http.Request, key string, def bool) bool {
	val := r.URL.Query().Get(key)
	if val == "" {
		return def
	}
	return val == "true"
}

// --- Perlin Noise ---

type Perlin struct{ p [512]int }

func NewPerlin(seed int64) *Perlin {
	rng := rand.New(rand.NewSource(seed))
	p := &Perlin{}
	var perm [256]int
	for i := range perm {
		perm[i] = i
	}
	rng.Shuffle(256, func(i, j int) { perm[i], perm[j] = perm[j], perm[i] })
	for i := 0; i < 512; i++ {
		p.p[i] = perm[i%256]
	}
	return p
}

func fade(t float64) float64 { return t * t * t * (t*(t*6-15) + 10) }
func lerp(t, a, b float64) float64 { return a + t*(b-a) }
func grad(hash int, x, y float64) float64 {
	h := hash & 3
	var u, v float64
	if h < 2 { u = x } else { u = y }
	if h == 0 || h == 2 { v = y } else { v = x }
	if (hash & 1) != 0 { u = -u }
	if (hash & 2) != 0 { v = -v }
	return u + v
}

func (p *Perlin) Noise2D(x, y float64) float64 {
	X := int(math.Floor(x)) & 255
	Y := int(math.Floor(y)) & 255
	x -= math.Floor(x)
	y -= math.Floor(y)
	u := fade(x)
	v := fade(y)
	A := p.p[X] + Y
	B := p.p[X+1] + Y
	hash00, hash01 := p.p[A], p.p[A+1]
	hash10, hash11 := p.p[B], p.p[B+1]
	lerpX1 := lerp(u, grad(hash00, x, y), grad(hash10, x-1, y))
	lerpX2 := lerp(u, grad(hash01, x, y-1), grad(hash11, x-1, y-1))
	return lerp(v, lerpX1, lerpX2)
}

func (p *Perlin) FBm(x, y float64, octaves int) float64 {
	var total float64
	freq, amp, max := 1.0, 1.0, 0.0
	for i := 0; i < octaves; i++ {
		total += p.Noise2D(x*freq, y*freq) * amp
		max += amp
		amp *= 0.5
		freq *= 2.0
	}
	return total / max
}

// --- HTML UI ---

const htmlUI = `
<!DOCTYPE html>
<html>
<head>
    <title>Digging Dwarfs Gen</title>
    <style>
        body { font-family: sans-serif; background: #1a1a1a; color: #fff; margin: 0; display: flex; height: 100vh; overflow: hidden; }
        #sidebar { width: 320px; background: #2a2a2a; padding: 20px; box-sizing: border-box; overflow-y: auto; z-index: 10; border-right: 1px solid #444; }
        #main { flex-grow: 1; position: relative; overflow: hidden; cursor: grab; background: #0a0a0a; }
        #main:active { cursor: grabbing; }

        .control-group { margin-bottom: 20px; }
        label { display: block; font-size: 14px; margin-bottom: 8px; color: #ccc; }
        input[type="number"] { width: 100%; padding: 8px; background: #333; color: #fff; border: 1px solid #555; border-radius: 4px; box-sizing: border-box; font-size: 16px; }
        input[type="number"]:focus { outline: none; border-color: #4CAF50; }

        .checkbox-label { display: flex; align-items: center; cursor: pointer; }
        .checkbox-label input { margin-right: 10px; width: 18px; height: 18px; }

        #preview { position: absolute; top: 0; left: 0; image-rendering: pixelated; transform-origin: 0 0; }
        .loading { opacity: 0.5; filter: grayscale(100%); }
        h2 { margin-top: 0; color: #4CAF50; }

		#controls-overlay { position: absolute; bottom: 20px; right: 20px; background: rgba(0,0,0,0.6); padding: 10px 15px; border-radius: 8px; pointer-events: none; color: #aaa; font-size: 13px; z-index: 5; }
    </style>
</head>
<body>
    <div id="sidebar">
        <h2>Map Tweaker</h2>

        <div class="control-group">
            <label>Block Size (Quantization)</label>
            <input type="number" id="blockSize" min="1" max="32" step="1" value="4" oninput="updateUI()">
        </div>

        <div class="control-group">
            <label>Cellular Automata Iterations</label>
            <input type="number" id="caIterations" min="0" max="10" step="1" value="2" oninput="updateUI()">
        </div>

        <div class="control-group">
            <label>Average Crust Thickness</label>
            <input type="number" id="crustThickness" min="10" max="200" step="1" value="35" oninput="updateUI()">
        </div>

        <div class="control-group">
            <label class="checkbox-label">
                <input type="checkbox" id="enablePitfalls" checked onchange="updateUI()">
                Enable Vertical Pitfalls
            </label>
        </div>

        <div class="control-group">
            <label class="checkbox-label">
                <input type="checkbox" id="enableEntrances" checked onchange="updateUI()">
                Enable Surface Entrances
            </label>
        </div>

        <p style="font-size: 12px; color: #888; margin-top: 40px; line-height: 1.5;">
            Note: Preview will automatically update after you stop typing.
        </p>
    </div>

    <div id="main">
        <img id="preview" src="/generate" alt="Map Preview">
		<div id="controls-overlay">Scroll to Zoom • Click & Drag to Pan</div>
    </div>

    <script>
        // --- Map Parameter Updating Logic ---
        let timeout = null;

        function updateUI() {
            clearTimeout(timeout);
            timeout = setTimeout(fetchMap, 500); // 500ms debounce
        }

        function fetchMap() {
            const preview = document.getElementById('preview');
            preview.classList.add('loading');

            const params = new URLSearchParams({
                blockSize: document.getElementById('blockSize').value,
                caIterations: document.getElementById('caIterations').value,
                crustThickness: document.getElementById('crustThickness').value,
                enablePitfalls: document.getElementById('enablePitfalls').checked,
                enableEntrances: document.getElementById('enableEntrances').checked
            });

            preview.src = "/generate?" + params.toString() + "&t=" + new Date().getTime();

            preview.onload = () => {
                preview.classList.remove('loading');
            };
        }

        // --- Zooming and Panning Logic ---
        const main = document.getElementById('main');
        const preview = document.getElementById('preview');

        let scale = 0.5; // Start slightly zoomed out to see more
        let pointX = 0;
        let pointY = 0;
        let start = { x: 0, y: 0 };
        let panning = false;

        // Prevent ghost drag on image
        preview.ondragstart = (e) => e.preventDefault();

        // Initial transform
        setTransform();

        main.addEventListener('mousedown', (e) => {
            e.preventDefault();
            start = { x: e.clientX - pointX, y: e.clientY - pointY };
            panning = true;
        });

        window.addEventListener('mouseup', () => {
            panning = false;
        });

        window.addEventListener('mousemove', (e) => {
            if (!panning) return;
            pointX = e.clientX - start.x;
            pointY = e.clientY - start.y;
            setTransform();
        });

        main.addEventListener('wheel', (e) => {
            e.preventDefault();

            // Get mouse position relative to the main container
            const rect = main.getBoundingClientRect();
            const mouseX = e.clientX - rect.left;
            const mouseY = e.clientY - rect.top;

            // Calculate mouse position on the image itself
            const xs = (mouseX - pointX) / scale;
            const ys = (mouseY - pointY) / scale;

            // Determine zoom direction
            const delta = e.deltaY > 0 ? -1 : 1;

            if (delta > 0) {
                scale *= 1.15; // Zoom in
            } else {
                scale /= 1.15; // Zoom out
            }

            // Clamp scale
            scale = Math.max(0.05, Math.min(scale, 10));

            // Adjust point X/Y to keep mouse stationary over image
            pointX = mouseX - xs * scale;
            pointY = mouseY - ys * scale;

            setTransform();
        }, { passive: false });

        function setTransform() {
            preview.style.transform = "translate(" + pointX + "px, " + pointY + "px) scale(" + scale + ")";
        }
    </script>
</body>
</html>
`
