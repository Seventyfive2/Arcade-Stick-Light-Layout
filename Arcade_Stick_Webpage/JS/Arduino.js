let gateSwitch
let colorInputs = [];

let inputField
let saveButton

let presets
let presetList = []

let ArduinoConnected = true;

function Setup()
{
    gateSwitch = document.getElementById("gateToggle");
    
    gateSwitch.addEventListener("click",ChangeGate);

    for (let i = 0; i < 8; i++)
    {
        colorInputs.push(document.getElementById("button"+i));
        colorInputs[i].addEventListener("change",function(){ ChangeButtonColor(i); });
    }

    saveButton = document.getElementById("save-Button")
    saveButton.addEventListener("click", SaveLayout);

    inputField = document.getElementById("lname")
    presets = document.getElementById("Preset-List");
    
    GetCurrentLayout();
}

function SendInput(cmd)
{
    const nocache = "&nocache=" + Math.random() * 1000000;
    const request = new XMLHttpRequest();

    request.open("Post", "ajax_inputs" + cmd + nocache, true);
    request.send(null);

    //console.log("ajax_inputs" + cmd + nocache);
}

function GetPresets()
{
    const nocache = "&nocache=" + Math.random() * 1000000;
    const request = new XMLHttpRequest();

    request.onreadystatechange = function()
    {
        if (this.readyState === 4 && this.status === 200)
        {
            if (this.responseXML != null)
            {
                let child = presets.lastElementChild
                while (child)
                {
                    presets.removeChild(child)
                    child = presets.lastElementChild
                }
                
                presetList = [];
                
                const num_an = this.responseXML.getElementsByTagName('layout').length;

                for (let i = 0; i < num_an; i++)
                {
                    let layout = this.responseXML.getElementsByTagName('layout')[i].childNodes[0].nodeValue;
                    layout = layout.trim();
                    
                    presetList.push(layout);
                    
                    const div = document.createElement("div")
                    div.className = "Saved-Preset"
                    
                    const name = document.createElement("p")
                    name.innerHTML = layout.substring(0,layout.length-4);
                    
                    const loadButton = document.createElement("button")
                    loadButton.innerHTML = "Load"
                    loadButton.addEventListener("click",function(){ LoadLayout(layout); })

                    const overWriteButton = document.createElement("button")
                    overWriteButton.innerHTML = "Overwrite"
                    overWriteButton.addEventListener("click",function(){ SaveLayout(layout); })

                    const deleteButton = document.createElement("button")
                    deleteButton.innerHTML = "Delete"
                    deleteButton.addEventListener("click",function(){ DeleteLayout(layout); })
                    
                    div.appendChild(name)
                    div.appendChild(loadButton)
                    div.appendChild(overWriteButton)
                    div.appendChild(deleteButton)
                    
                    presets.appendChild(div)
                    
                    //console.log(layout)
                }
            }
            else
            {
                console.log("Invalid XML Response");
                console.log(this.responseText);
            }
        }
        else if(request.status === 404)
        {
            let child = presets.lastElementChild
            while (child)
            {
                presets.removeChild(child)
                child = presets.lastElementChild
            }
            
            const dummyList = ["Sample.xml","Test.xml","Tekken.xml","SNES.xml","Gamecube.xml"]
            
            for (let i = 0; i < dummyList.length; i++)
            {
                const layout = dummyList[i];
                
                const div = document.createElement("div")
                div.className = "Saved-Preset"

                presets.appendChild(div)

                const name = document.createElement("p")
                name.innerHTML = layout.substring(0,layout.length-4);

                div.appendChild(name)

                const inputs = document.createElement("div")
                inputs.className = "Preset-Actions"

                div.append(inputs)

                const loadButton = document.createElement("button")
                loadButton.innerHTML = "Load"
                loadButton.addEventListener("click",function(){ LoadLayout(dummyList[i]); })

                const overWriteButton = document.createElement("button")
                overWriteButton.innerHTML = "Overwrite"
                overWriteButton.addEventListener("click",function(){ SaveLayout(dummyList[i]); })

                const deleteButton = document.createElement("button")
                deleteButton.innerHTML = "Delete"
                deleteButton.addEventListener("click",function(){ DeleteLayout(dummyList[i]); })
                
                inputs.appendChild(loadButton)
                inputs.appendChild(overWriteButton)
                inputs.appendChild(deleteButton)

                //console.log(layout)
            }
        }
    }

    request.open("Get", "presets" + nocache, true);
    request.send(null);
}

function GetCurrentLayout()
{
    const nocache = "&nocache=" + Math.random() * 1000000;
    const request = new XMLHttpRequest(); 
    request.onreadystatechange = function()
    {
        if (this.readyState === 4 && this.status === 200)
        {
            if (this.responseXML != null)
            {
                // XML file received - contains analog values, switch values and LED states
                let gateState = this.responseXML.getElementsByTagName('fourWay')[0].childNodes[0].nodeValue;

                gateState = gateState.trim();
                
                gateSwitch.checked = gateState === "true";
                console.log("gateState = " + gateState + " GateSwitch = " + gateSwitch.checked);
                
                //gateSwitch.value = Boolean(gateState);

                // get button inputs
                const num_an = this.responseXML.getElementsByTagName('button').length;

                for (let i = 0; i < num_an; i++)
                {
                    if(colorInputs[i].id !==document.activeElement.id)
                    {
                        let color = this.responseXML.getElementsByTagName('button')[i].childNodes[0].nodeValue;
                        color = color.trim();
                        colorInputs[i].value = color;
                    }
                }
            }
            else
            {
                console.log("Invalid XML Response");
                console.log(this.responseText);
            }
        }
        else if(request.status === 404)
        {
            console.log("Server Down"); 
            ArduinoConnected = false;
        }
    }
    
    request.open("Get", "layout" + nocache, true);
    request.send(null);
    
    if(ArduinoConnected)
    {
        GetPresets();
        setTimeout('GetCurrentLayout()', 5000);
    }
}
//Toggle Gate when checkbox checked/unchecked
function ChangeGate()
{
    let input = "&4Way=";
    
    if (gateSwitch.checked)
    {
        input += "true";
    }
    else 
    {
        input += "false";
    }
    
    SendInput(input);
}

function ChangeButtonColor(id)
{
    const input = "&BTN" + id.toString() + "=" + colorInputs[id].value.substring(1);
    SendInput(input);
}

function SaveLayout()
{
    if(inputField.value !== "")
    {
        SendInput("&Save="+inputField.value)
    }
}

function LoadLayout(layoutName)
{
    SendInput("&Load="+layoutName)
}

function DeleteLayout(layoutName)
{
    SendInput("&Delete="+layoutName)
}