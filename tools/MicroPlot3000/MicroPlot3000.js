/* 
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the 'License');
you may not use $ file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an 'AS IS' BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


// The idea to normalize the data for display and use html for the axis is brilliant:
//  https://dev.to/richharris/a-new-technique-for-making-responsive-javascript-free-charts-gmp


MicroPlot3000 = new function() {
    let $ = this;

    $.init = function(urlFolder, ylabel, dataIndexY = 1, domElementId = 'plot') {
        // To be called onLoad
        $.urlFolder = urlFolder;
        $.ylabel = ylabel;
        $.dataIndexY = dataIndexY;

        // Append the HTML and CSS to the DOM
        $.domElement = document.getElementById(domElementId);
        $.domElement.insertAdjacentHTML('beforeend', $.htmlContent);

        // Load the data
        (async () => await $.loadData())();
    };

    $.urlFolder;
    $.ylabel = '';
    $.dataIndexY;
    
    $.domElement;

    $.data = [];

    $.loadData = async function() {
        try {
            const url = `http://${location.host}/${$.urlFolder}`;
            const response = await fetch(url);
            if (!response.ok)
                throw new Error(`Response status: ${response.status}`);

            let csvString = await response.text();
            $.appendData(csvString);

        } catch (error) {
            console.error(error.message);
        }
    };

    $.appendData = function(csvString) {
        // Remove all whitespaces, newline characters, etc.
        csvString = csvString.replace(/\s/g, '');
        // Remove the last semicolon
        csvString = csvString.slice(0, -1);
        
        // Split the data string into an array of objects
        csvString.split(';').forEach(row => {
            let values = row.split(',').map(Number);
            let x = values[0];
            let y = values[$.dataIndexY];
            $.data.push({x, y});
        });

        $.plotData();
    };

    $.plotData = function() {
        // Find the max/min x/y values
        let xMax = Math.max(...$.data.map(d => d.x));
        let xMin = Math.min(...$.data.map(d => d.x));
        let yMax = Math.max(...$.data.map(d => d.y));
        let yMin = Math.min(...$.data.map(d => d.y));

        // Scale the data to fit within the svg
        let dataScaled = $.data.map(d => {
            return {
                x: (d.x - xMin) / (xMax - xMin) * 100,
                y: 100 - (d.y - yMin) / (yMax - yMin) * 100
            }
        });

        // Update the polyline points
        $.domElement.querySelector('polyline').setAttribute('points', dataScaled.map(d => `${d.x},${d.y}`).join(' '));

        // Update the time axis label
        let timeDiff = xMax - xMin;
        let dtStart = new Date(xMin);

        // Update the time axis ticks depending on the time span
        let unitString = '';
        $.domElement.querySelectorAll('.xaxis span').forEach((span, i) => {
            let number = xMin + (xMax - xMin) / 5 * i;
            let dt = new Date(number);
            if (timeDiff < 10*60*1000) {
                // < 10 m -> tick: m:ss
                span.textContent = `${dt.getUTCMinutes()}:${dt.getUTCSeconds().toString().padStart(2, '0')}`;
                unitString = '[m:ss]';
            } else if (timeDiff < 10*60*60*1000) {
                // < 10 h -> tick: h:mm
                span.textContent = `${dt.getUTCHours()}:${dt.getUTCMinutes().toString().padStart(2, '0')}`;
                unitString = '[h:mm]';
            } else if (timeDiff < 10*24*60*60*1000) {
                // < 10 d -> tick: d hh
                span.textContent = `${dt.getUTCDate()} ${dt.getUTCHours().toString().padStart(2, '0')}h`;
                unitString = '[d hh]h';
            } else if (timeDiff < 10*30*24*60*60*1000) {
                // < 10 M -> tick: yyyy-MM
                span.textContent = `${(dt.getUTCMonth() + 1)}-${dt.getUTCDate().toString().padStart(2, '0')}`;
                unitString = '[MM-dd]';
            } else {
                // tick: yyyy-MM-dd
                span.textContent = `${dt.getUTCFullYear()}-${(dt.getUTCMonth() + 1).toString().padStart(2, '0')}-${dt.getUTCDate().toString().padStart(2, '0')}`;
                unitString = '[yyyy-MM-dd]';
            }
        });
        $.domElement.querySelector('.xaxis div').textContent = `from UTC ${dtStart.getFullYear()}-${(dtStart.getUTCMonth() + 1).toString().padStart(2, '0')}-${dtStart.getUTCDate().toString().padStart(2, '0')} ${dtStart.getUTCHours().toString().padStart(2, '0')}:${dtStart.getUTCMinutes().toString().padStart(2, '0')}:${dtStart.getUTCSeconds().toString().padStart(2, '0')} as ${unitString}`;

        // Get the order of magnitude for y axis labeling 
        let magnitudeY = Math.log10(Math.abs(yMax - yMin)) | 0;
        let precisionY = Math.max(0, 1 - magnitudeY); // Cannot be < 0
        if (magnitudeY < 0)
            precisionY++;

        // Very crude way to make sure the y axis ticks are readable
        let ylabel = $.ylabel;
        if (magnitudeY > 3) {
            yMin = yMin / 1000;
            yMax = yMax / 1000;
            ylabel += ' (E+3)';
        }
        if (magnitudeY < 0) {
            yMin = yMin * 1000;
            yMax = yMax * 1000;
            precisionY -= 3;
            ylabel += ' (E-3)';
        }
        $.domElement.querySelector('.yaxis div div').textContent = ylabel;

        // Assign values to the y axis using the min/max values
        $.domElement.querySelectorAll('.yaxis span').forEach((span, i) => {
            let number = yMin + (yMax - yMin) / 5 * i;
            span.textContent = number.toFixed(precisionY);
        });
    };

    $.htmlContent = `
<style>

    /* Outer envelope */
    .chart-outer {
        position: relative;
        width: 100%;
        height: 100%;
        padding-bottom: 2em;
        padding-left: 3.5em;
        padding-right: 0.5em;
        box-sizing: border-box;
    }

    /* Inner envelope */
    .chart-inner {
        position: relative;
        width: 100%;
        height: 100%;
        font-size: small;
    }
    
    /* Axis lines */
    .xaxis {
        position: absolute;
        top: 100%;
        width: 100%;
        border-top: 1px solid black;
        text-align: center;
        line-height: 1;
    }

    .yaxis {
        position: absolute;
        left: 0;
        height: 100%;
        border-left: 1px solid black;
        line-height: 1;
    }

    /* Ticks */
    span:nth-child(1) { --index: 0; }
    span:nth-child(2) { --index: 1; }
    span:nth-child(3) { --index: 2; }
    span:nth-child(4) { --index: 3; }
    span:nth-child(5) { --index: 4; }
    span:nth-child(6) { --index: 5; }

    .xaxis > span {
        position: absolute;
        line-height: 1;
        left: calc(var(--index)*20%);
        top: 0.5em;
        transform: translate(-50%,0);
    }

    .yaxis > span {
        position: absolute;
        line-height: 1;
        left: -0.5em;
        top: calc(100% - var(--index)*20%);
        transform: translate(-100%,-50%);
    }

    /* Labels */
    .xaxis div {
        margin-top: 2em;
    }

    .yaxis > div {
        position: absolute;
        height: 100%;
        transform: translate(-5em, 50%) rotate(-90deg);
        transform-origin: top;
    }

    .yaxis > div > div {
        position: absolute;
        white-space: nowrap;
        transform: translate(-50%);
    }

    /* Graph SVG */
    .graph {
        position: absolute;
        width: 100%;
        height: 100%;
        overflow: visible;
    }
    
    .graph * {
        vector-effect: non-scaling-stroke;
    }
    
    .graph polyline {
        fill: none;
        stroke: red;
        stroke-width: 2;
    }

    .graph line {
        stroke: grey;
        stroke-width: 0.5;
        stroke-dasharray: 2,2;
    }

</style>
<div class='chart-outer'>
    <div class='chart-inner'>
        <div class='xaxis'>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <div></div>
        </div>
        <div class='yaxis'>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <span></span>
            <div><div></div></div>
        </div>
        <svg class='graph' viewBox='0 0 100 100' preserveAspectRatio='none'>
            <g>
                <!-- Vertical lines -->
                <line x1='20' y1='0' x2='20' y2='100'></line>
                <line x1='40' y1='0' x2='40' y2='100'></line>
                <line x1='60' y1='0' x2='60' y2='100'></line>
                <line x1='80' y1='0' x2='80' y2='100'></line>
                <line x1='100' y1='0' x2='100' y2='100'></line>
                <!-- Horizontal lines, from top with 0 -->
                <line x1='0' y1='0' x2='100' y2='0'></line>
                <line x1='0' y1='20' x2='100' y2='20'></line>
                <line x1='0' y1='40' x2='100' y2='40'></line>
                <line x1='0' y1='60' x2='100' y2='60'></line>
                <line x1='0' y1='80' x2='100' y2='80'></line>
            </g>
            <!-- Data -->
            <polyline points=''></polyline>
        </svg>
    </div>
</div>
    `;

};