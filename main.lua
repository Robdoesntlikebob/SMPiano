local love = require ("love")
local loveframes = require ("loveframes")
local changelog = "Step 2: Implementing SNES emulation"
local SNES = require ("SNES")

local songs = {}
local samples = {}
local app = {}
local relwidth, relheight = love.window.getMode()

--[[
function ()
        local stnframe = loveframes.Create("frame")
        stnframe:SetName("Edit Sample "..name)
        stnframe:SetSize(relwidth*0.6, relheight*0.4)
        stnframe:Center()
        local gadsr = loveframes.Create("form", stnframe)
        gadsr:SetName("GAIN/ADSR Values")
        gadsr:SetPos(5,25)
        gadsr:SetSize(stnframe:GetWidth() - 10, (stnframe:GetHeight()/2)-25)
        local tuning = loveframes.Create("form", stnframe)
        tuning:SetName("Tuning")
        tuning:SetPos(5, (stnframe:GetHeight()/2))
        tuning:SetSize(stnframe:GetWidth() - 10, (stnframe:GetHeight()/2)-5)
    end]]


local function drawsmp(chosensample, name, sound, path, inst, settings)
    path = chosensample[1]
    if path == nil then app.import:SetClickable(true) return else _, name = string.match(path, "(.-)([^\\]-([^\\%.]+))$") end
    SNES.brr2aram(path)
    local smpBTN = loveframes.Create("button", app.smpframe)
    smpBTN:SetText(name)
    smpBTN.OnClick = function()
        SNES.play(path)
    end 
    --allow importing another file
    app.import:SetClickable(true)
end

local function addsample()
    app.x = love.window.showFileDialog("openfile", drawsmp, {title = "Import Sample", acceptlabel = "Import", cancellabel = "Back", attachtowindow=false, multiselect = false, filters = {["BRR file (*.brr)"] = "brr",["WAV file (*.wav)"] = "wav" }})
end


function app.maintop()
    app.mainpanel = loveframes.Create("panel")
    app.mainpanel:SetSize(relwidth, 35)

    app.sect1 = loveframes.Create("grid", app.mainpanel)
    app.sect1:SetVisible(true)
    app.sect1:SetRows(1)
    app.sect1:SetColumns(2)
    app.settings = loveframes.Create("imagebutton")
    app.settings:SetImage("stng.png")
    app.sect1:AddItem(app.settings, 1, 2, "right")
    app.settings:SetText("")
    app.settings:SizeToImage()
    app.settings.OnClick = function ()
        app.settingswindow()
        app.settings:SetClickable(false)
    end
    app.import = loveframes.Create("imagebutton", app.smpframe)
    app.import:SetImage("fileprot.png")
    app.import:SetText("")
    app.import:SizeToImage()
    app.sect1:AddItem(app.import, 1,1,"left")
    app.import.OnClick = function ()
        app.import:SetClickable(false)
        addsample()
    end
    --make sure this always stays at the end so everything is aligned properly
    app.sect1:SetItemAutoSize(true)
end

function app.channels()
    app.chpanel = loveframes.Create("list")
    app.chpanel:SetY(app.mainpanel:GetHeight())
    app.chpanel:SetSize(app.mainpanel:GetWidth()-app.snigeneral:GetWidth(), app.mainpanel:GetHeight()*4)
end

function app.sni()
    app.snigeneral = loveframes.Create("list")
    app.snigeneral:SetSize(225,relheight-35)
    app.snigeneral:SetPos(relwidth-225,35)

    app.songs = loveframes.Create("collapsiblecategory", app.snigeneral)
    app.songs:SetText("Songs")
    app.sngframe = loveframes.Create("list")
    app.songs:SetObject(app.sngframe)
    app.samples = loveframes.Create("collapsiblecategory", app.snigeneral)
    app.smpframe = loveframes.Create("list")
    app.samples:SetObject(app.smpframe)
    app.samples:SetText("Samples")
    app.instruments = loveframes.Create("collapsiblecategory", app.snigeneral)
    app.instframe = loveframes.Create("list")
    app.instruments:SetObject(app.instframe)
    app.instruments:SetText("Instruments")
    app.instruments:SetOpen(true)
    app.songs:SetOpen(true)
    app.samples:SetOpen(true)
end


--Settings window contains: Theme, Global Tuning, Sound Mode (GBA, SNES)
function app.settingswindow()
    app.stWINDOW = loveframes.Create("frame")
    app.stWINDOW:SetName("Settings")
    app.stWINDOW:SetSize(relwidth*0.8, relheight*0.8)
    app.stWINDOW:Center()
    app.stWINDOW.OnClose = function ()
        app.settings:SetClickable(true)
    end
end

function love.load()
    love.graphics.setDefaultFilter("nearest", "nearest")
    love.graphics.setBackgroundColor(0xb5/255,0xb6/255,0xe4/255)
    app.maintop()
    app.sni()
    app.channels()
    SNES.load()
end

function love.update(dt)
    loveframes.update(dt)
end

function love.draw()
    loveframes.draw()
end

function love.mousepressed(x,y,button)
    loveframes.mousepressed(x,y,button)
end

function love.mousereleased(x, y, button)
    loveframes.mousereleased(x, y, button)
end

function love.keypressed(key, scancode, isrepeat)
    loveframes.keypressed(key, scancode)
end

function love.keyreleased(key)
    loveframes.keyreleased(key)
end

function love.resize(w,h)
end